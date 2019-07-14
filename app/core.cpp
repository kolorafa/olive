/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "core.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QHBoxLayout>

#include "panel/panelfocusmanager.h"
#include "panel/project/project.h"
#include "project/item/footage/footage.h"
#include "task/import/import.h"
#include "task/taskmanager.h"
#include "ui/icons/icons.h"
#include "ui/style/style.h"
#include "undo/undostack.h"
#include "widget/menu/menushared.h"
#include "widget/taskview/taskviewitem.h"

Core olive::core;

Core::Core() :
  main_window_(nullptr),
  tool_(olive::tool::kPointer)
{
}

void Core::Start()
{
  //
  // Parse command line arguments
  //

  QCoreApplication* app = QCoreApplication::instance();

  QCommandLineParser parser;
  parser.addHelpOption();
  parser.addVersionOption();

  // Project from command line option
  // FIXME: What's the correct way to make a visually "optional" positional argument, or is manually adding square
  // brackets like this correct?
  parser.addPositionalArgument("[project]", tr("Project to open on startup"));

  // Create fullscreen option
  QCommandLineOption fullscreen_option({"f", "fullscreen"}, tr("Start in full screen mode"));
  parser.addOption(fullscreen_option);

  // Parse options
  parser.process(*app);

  QStringList args = parser.positionalArguments();

  // Detect project to load on startup
  if (!args.isEmpty()) {
    startup_project_ = args.first();
  }

  // Declare custom types for Qt signal/slot syste
  DeclareTypesForQt();


  //
  // Start GUI (TODO CLI mode)
  //

  StartGUI(parser.isSet(fullscreen_option));

  // Create new project on startup
  // TODO: Load project from startup_project_ instead if not empty
  AddOpenProject(std::make_shared<Project>());
}

void Core::Stop()
{
  delete main_window_;
}

olive::MainWindow *Core::main_window()
{
  return main_window_;
}

void Core::ImportFiles(const QStringList &urls, ProjectViewModel* model, Folder* parent)
{
  if (urls.isEmpty()) {
    QMessageBox::critical(main_window_, tr("Import error"), tr("Nothing to import"));
    return;
  }

  olive::task_manager.AddTask(std::make_shared<ImportTask>(model, parent, urls));
}

const olive::tool::Tool &Core::tool()
{
  return tool_;
}

void Core::StartModalTask(Task *t)
{
  QDialog dialog(main_window_);

  QHBoxLayout* layout = new QHBoxLayout(&dialog);
  layout->setMargin(0);

  TaskViewItem* task_view = new TaskViewItem(&dialog);
  task_view->SetTask(t);
  layout->addWidget(task_view);

  connect(t, SIGNAL(Finished()), &dialog, SLOT(accept()));

  // FIXME: Risk of task finishing before dialog execs?

  if (t->Start()) {
    dialog.exec();
  }
}

void Core::SetTool(const olive::tool::Tool &tool)
{
  tool_ = tool;

  emit ToolChanged(tool_);
}

void Core::DialogImportShow()
{
  // Open dialog for user to select files
  QStringList files = QFileDialog::getOpenFileNames(main_window_,
                                                    tr("Import footage..."));

  // Check if the user actually selected files to import
  if (!files.isEmpty()) {

    // Locate the most recently focused Project panel (assume that's the panel the user wants to import into)
    ProjectPanel* active_project_panel = olive::panel_focus_manager->MostRecentlyFocused<ProjectPanel>();
    Project* active_project;

    if (active_project_panel == nullptr // Check that we found a Project panel
        || (active_project = active_project_panel->project()) == nullptr) { // and that we could find an active Project
      QMessageBox::critical(main_window_, tr("Failed to import footage"), tr("Failed to find active Project panel"));
      return;
    }

    // Get the selected folder in this panel
    Folder* folder = active_project_panel->GetSelectedFolder();

    ImportFiles(files, active_project_panel->model(), folder);
  }
}

void Core::CreateNewFolder()
{
  // Locate the most recently focused Project panel (assume that's the panel the user wants to import into)
  ProjectPanel* active_project_panel = olive::panel_focus_manager->MostRecentlyFocused<ProjectPanel>();
  Project* active_project;

  if (active_project_panel == nullptr // Check that we found a Project panel
      || (active_project = active_project_panel->project()) == nullptr) { // and that we could find an active Project
    QMessageBox::critical(main_window_, tr("Failed to create new folder"), tr("Failed to find active Project panel"));
    return;
  }

  // Get the selected folder in this panel
  Folder* folder = active_project_panel->GetSelectedFolder();

  // Create new folder
  ItemPtr new_folder = std::make_shared<Folder>();

  // Set a default name
  new_folder->set_name(tr("New Folder"));

  // Create an undoable command
  ProjectViewModel::AddItemCommand* aic = new ProjectViewModel::AddItemCommand(active_project_panel->model(),
                                                                               folder,
                                                                               new_folder);

  olive::undo_stack.push(aic);

  // Trigger an automatic rename so users can enter the folder name
  active_project_panel->Edit(new_folder.get());
}

void Core::AddOpenProject(ProjectPtr p)
{
  open_projects_.append(p);

  emit ProjectOpened(p.get());
}

void Core::DeclareTypesForQt()
{
  qRegisterMetaType<Task::Status>("Task::Status");
}

void Core::StartGUI(bool full_screen)
{
  // Load all icons
  olive::icon::LoadAll();

  // Set UI style
  olive::style::AppSetDefault();

  // Set up shared menus
  olive::menu_shared.Initialize();

  // Since we're starting GUI mode, create a PanelFocusManager (auto-deletes with QObject)
  olive::panel_focus_manager = new PanelFocusManager(this);

  // Connect the PanelFocusManager to the application's focus change signal
  connect(qApp,
          SIGNAL(focusChanged(QWidget*, QWidget*)),
          olive::panel_focus_manager,
          SLOT(FocusChanged(QWidget*, QWidget*)));

  // Create main window and open it
  main_window_ = new olive::MainWindow();
  if (full_screen) {
    main_window_->showFullScreen();
  } else {
    main_window_->showMaximized();
  }

  // When a new project is opened, update the mainwindow
  connect(this, SIGNAL(ProjectOpened(Project*)), main_window_, SLOT(ProjectOpen(Project*)));


}

Project *Core::GetActiveProject()
{
  // Locate the most recently focused Project panel (assume that's the panel the user wants to import into)
  ProjectPanel* active_project_panel = olive::panel_focus_manager->MostRecentlyFocused<ProjectPanel>();

  // If we couldn't find one, return nullptr
  if (active_project_panel == nullptr) {
    return nullptr;
  }

  // Otherwise, return the project panel's project (which may be nullptr but in most cases shouldn't be)
  return active_project_panel->project();
}