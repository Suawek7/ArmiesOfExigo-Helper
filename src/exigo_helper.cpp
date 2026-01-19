#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <windows.h>
#include <sstream>

namespace fs = std::filesystem;

std::string gamePath;
std::string gameExecutable = "Exigo.exe";
std::string editorExecutable = "";
std::string appVersion = "0.1";

// DevTools configuration.
bool devToolsEnabled = false;
std::string devToolsORKBuildSourcePath = "";
std::string devToolsORKBuildTargetPath = "";
std::string devToolsORKBuildFileListPath = "";
std::string DevToolsExigoSpeedhackInitiatorExecutableName = "";
int devToolsExigoSpeedhackValue = 5;  // Default value 5x
std::string devToolsORKCmpPath = "";

// Set console text color.
void setConsoleColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

// Trim whitespace from string.
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

// Change SpeedHack value in DevTools.conf.
void changeSpeedHackValue() {
    std::string configFile = "eh_devtools/DevTools.conf";
    
    if (!fs::exists(configFile)) {
        setConsoleColor(12); // Red
        std::cout << "Error: DevTools.conf not found!" << std::endl;
        setConsoleColor(7);
        std::cin.ignore();
        std::cout << "Press Enter to continue...";
        std::cin.get();
        return;
    }
    
    setConsoleColor(11); // Cyan
    std::cout << "Current SpeedHack value: " << devToolsExigoSpeedhackValue << "x" << std::endl;
    setConsoleColor(7);
    
    int newValue;
    std::cout << "Enter new SpeedHack value: ";
    std::cin >> newValue;
    
    // Read current config file content.
    std::ifstream fileIn(configFile);
    std::string content((std::istreambuf_iterator<char>(fileIn)),
                        std::istreambuf_iterator<char>());
    fileIn.close();
    
    // Check if DevToolsExigoSpeedhackValue entry exists.
    size_t pos = content.find("DevToolsExigoSpeedhackValue=");
    
    if (pos != std::string::npos) {
        // Replace existing value.
        size_t lineEnd = content.find('\n', pos);
        if (lineEnd == std::string::npos) {
            lineEnd = content.length();
        }
        
        std::string newLine = "DevToolsExigoSpeedhackValue=" + std::to_string(newValue);
        content.replace(pos, lineEnd - pos, newLine);
    } else {
        // Add new entry at the end.
        if (!content.empty() && content.back() != '\n') {
            content += "\n";
        }
        content += "DevToolsExigoSpeedhackValue=" + std::to_string(newValue) + "\n";
    }
    
    // Write updated content back to file.
    std::ofstream fileOut(configFile);
    fileOut << content;
    fileOut.close();
    
    // Update global variable.
    devToolsExigoSpeedhackValue = newValue;
    
    setConsoleColor(10); // Green
    std::cout << "SpeedHack value changed to " << newValue << "x" << std::endl;
    setConsoleColor(7);
    
    std::cin.ignore();
    std::cout << "Press Enter to continue...";
    std::cin.get();
}

// Extract value from quoted string (e.g., "value" from key="value").
std::string extractQuotedValue(const std::string& line) {
    size_t firstQuote = line.find('"');
    if (firstQuote == std::string::npos) return "";
    
    size_t secondQuote = line.find('"', firstQuote + 1);
    if (secondQuote == std::string::npos) return "";
    
    return line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
}

// Extract boolean value from line (e.g., "true" or "false").
bool extractBoolValue(const std::string& line) {
    size_t equalPos = line.find('=');
    if (equalPos == std::string::npos) return false;
    
    std::string value = line.substr(equalPos + 1);
    value = trim(value);
    
    return (value == "true" || value == "True" || value == "TRUE");
}

// Load DevTools configuration.
void loadDevToolsConfig() {
    std::string configFile = "eh_devtools/DevTools.conf";
    
    if (!fs::exists(configFile)) {
        return; // DevTools config doesn't exist, keep disabled.
    }
    
    std::ifstream file(configFile);
    std::string line;
    
    while (std::getline(file, line)) {
        line = trim(line);
        
        if (line.find("DevTools=") == 0) {
            devToolsEnabled = extractBoolValue(line);
        }
        else if (line.find("DevToolsORKBuildSourcePath=") == 0) {
            devToolsORKBuildSourcePath = extractQuotedValue(line);
        }
        else if (line.find("DevToolsORKBuildTargetPath=") == 0) {
            devToolsORKBuildTargetPath = extractQuotedValue(line);
        }
        else if (line.find("DevToolsORKBuildFileListPath=") == 0) {
            devToolsORKBuildFileListPath = extractQuotedValue(line);
        }
        else if (line.find("DevToolsExigoSpeedhackInitiatorExecutableName=") == 0) {
            DevToolsExigoSpeedhackInitiatorExecutableName = extractQuotedValue(line);
        }
        else if (line.find("DevToolsExigoSpeedhackValue=") == 0) {
            // Extract numeric value (without quotes).
            size_t equalPos = line.find('=');
            if (equalPos != std::string::npos) {
                std::string valueStr = trim(line.substr(equalPos + 1));
                int val = std::stoi(valueStr);
                devToolsExigoSpeedhackValue = val;
            }
        }
        else if (line.find("DevToolsORKCmpPath=") == 0) {
            devToolsORKCmpPath = extractQuotedValue(line);
        }
    }
    
    file.close();
    
    if (devToolsEnabled) {
        setConsoleColor(11);
        std::cout << "DevTools enabled" << std::endl;
        setConsoleColor(7);
    }
}

// Load game path from config file and verify executables.
// Returns true if all required files are found, false otherwise.
bool loadGamePath() {
    std::string configFile = "ExigoHelper.conf";
    bool pathValid = false;
    
    if (fs::exists(configFile)) {
        std::ifstream file(configFile);
        std::string line;
        
        while (std::getline(file, line)) {
            line = trim(line);
            if (line.find("ExigoSystemPath=") == 0) {
                std::string path = extractQuotedValue(line);
                if (!path.empty()) {
                    if(fs::exists(path)) {
                        gamePath = path;
                        pathValid = true;
                        setConsoleColor(10);
                        std::cout << "Loaded game path from config: " << gamePath << std::endl;
                        setConsoleColor(7);
                    } else {
                        setConsoleColor(15);
                        std::cout << "Error: Path from config file: " << path << " does not exists in the system!" << std::endl;
                        std::cout << "Fallback to actual game location.." << std::endl;
                        setConsoleColor(7);
                    }
                }
            }
            // Look for game executable name.
            if (line.find("ExigoGameExecutable=") == 0) {
                std::string name = extractQuotedValue(line);
                if (!name.empty()) {
                    gameExecutable = name;
                }
            }
            // Look for editor executable name.
            if (line.find("ExigoEditorExecutable=") == 0) {
                std::string name = extractQuotedValue(line);
                if (!name.empty()) {
                    editorExecutable = name;
                }
            }
        }
        file.close();
    }
    
    // Fallback to current directory if no path was loaded.
    if (gamePath.empty()) {
        gamePath = fs::current_path().string();
        pathValid = true;
        setConsoleColor(14);
        std::cout << "Using current directory as game path: " << gamePath << std::endl;
        setConsoleColor(7);
    }
    if (!pathValid) {
        setConsoleColor(12);
        std::cout << "\nFATAL ERROR: Invalid game path!" << std::endl;
        setConsoleColor(7);
        return false;
    }
    
    // Verify game executable exists.
    std::string gameExePath = gamePath + "\\" + gameExecutable;
    bool gameExeFound = false;
    if (fs::exists(gameExePath)) {
        setConsoleColor(10);
        gameExeFound = true;
        std::cout << "Game executable found: " << gameExecutable << std::endl;
        setConsoleColor(7);
    } else {
        setConsoleColor(12);
        std::cout << "ERROR: Game executable not found: " << gameExePath << std::endl;
        setConsoleColor(7);
    }
    
    // Verify editor executable exists (if specified).
    bool editorExeValid = true;
    if (!editorExecutable.empty()) {
        std::string editorExePath = gamePath + "\\" + editorExecutable;
        if (fs::exists(editorExePath)) {
            setConsoleColor(10);
            std::cout << "Editor executable found: " << editorExecutable << std::endl;
            setConsoleColor(7);
        } else {
            editorExeValid = false;
            setConsoleColor(12);
            std::cout << "ERROR: Editor executable not found: " << editorExePath << std::endl;
            setConsoleColor(7);
        }
    }

    // Check DataORKs folder.
    std::string dataOrksPath = gamePath + "\\DataORKs";
    bool dataOrksFolderFound = false;
    if (fs::exists(dataOrksPath) && fs::is_directory(dataOrksPath)) {
        dataOrksFolderFound = true;
        setConsoleColor(10);
        std::cout << "DataORKs folder found" << std::endl;
        setConsoleColor(7);
    } else {
        setConsoleColor(12);
        std::cout << "ERROR: DataORKs folder not found: " << dataOrksPath << std::endl;
        setConsoleColor(7);
    }
    
    // Return true only if all required components are found.
    return gameExeFound && editorExeValid && dataOrksFolderFound;
}

// Get full path by combining game path with relative path.
std::string getFullPath(const std::string& relativePath) {
    return gamePath + "\\" + relativePath;
}

// Display current game version from version file.
void showCurrentVersion() {
    std::string versionFile = getFullPath("DataORKs\\Game_version.txt");
    
    if (fs::exists(versionFile)) {
        try {
            std::ifstream file(versionFile);
            std::string currentVersion;
            std::getline(file, currentVersion);
            file.close();
            
            setConsoleColor(10); // Green
            std::cout << "Actual game version: " << currentVersion << std::endl;
            setConsoleColor(7); // Default
        }
        catch (...) {
            setConsoleColor(14); // Yellow
            std::cout << "Actual game version: Unknown (could not read version file)" << std::endl;
            setConsoleColor(7);
        }
    }
}

void removeDataFiles() {
    setConsoleColor(14); // Yellow
    // Remove all DataX.ork files (X = 1 to 9).
    std::cout << "Removing existing DataX.ork files..." << std::endl;
    setConsoleColor(7);
    
    for (int i = 1; i <= 9; i++) {
        std::string fileName = getFullPath("Data" + std::to_string(i) + ".ork");
        if (fs::exists(fileName)) {
            fs::remove(fileName);
            setConsoleColor(8); // Gray
            std::cout << "Removed: Data" << i << ".ork" << std::endl;
            setConsoleColor(7);
        }
    }
}

void copyDataFiles(const std::string& sourceFolder, const std::string& versionName) {
    setConsoleColor(14); // Yellow
    // Copy data files from selected version folder.
    std::cout << "Copying files from " << sourceFolder << "..." << std::endl;
    setConsoleColor(7);
    
    for (int i = 1; i <= 9; i++) {
        std::string sourceFile = sourceFolder + "\\Data" + std::to_string(i) + ".ork";
        std::string destFile = getFullPath("Data" + std::to_string(i) + ".ork");
        
        if (fs::exists(sourceFile)) {
            fs::copy_file(sourceFile, destFile, fs::copy_options::overwrite_existing);
            setConsoleColor(8); // Gray
            std::cout << "Copied: Data" << i << ".ork" << std::endl;
            setConsoleColor(7);
        }
    }
    
    // Save version to file.
    std::string versionFile = getFullPath("DataORKs\\Game_version.txt");
    if (fs::exists(versionFile)) {
        fs::remove(versionFile);
    }
    
    std::ofstream file(versionFile);
    file << versionName;
    file.close();
    
    // Set file as hidden.
    if (fs::exists(versionFile)) {
        SetFileAttributesA(versionFile.c_str(), FILE_ATTRIBUTE_HIDDEN);
    }
    
    setConsoleColor(10); // Green
    std::cout << "\nGame is now using version: " << versionName << std::endl;
    setConsoleColor(7);
}

// Handle version change menu and operations.
void handleVersionChange() {
    std::string dataOrksPath = getFullPath("DataORKs");
    
    // Check if DataORKs folder exists
    if (!fs::exists(dataOrksPath)) {
        setConsoleColor(12); // Red
        std::cout << "Error: DataORKs folder not found!" << std::endl;
        setConsoleColor(7);
        std::cin.ignore();
        std::cout << "Press Enter to continue...";
        std::cin.get();
        return;
    }
    
    // Get list of version folders
    std::vector<std::string> folders;
    for (const auto& entry : fs::directory_iterator(dataOrksPath)) {
        if (entry.is_directory()) {
            folders.push_back(entry.path().filename().string());
        }
    }

    if (folders.empty()) {
        setConsoleColor(12); // Red
        std::cout << "No version folders found in DataORKs!" << std::endl;
        setConsoleColor(7);
        std::cin.ignore();
        std::cout << "Press Enter to continue...";
        std::cin.get();
        return;
    }
    
    setConsoleColor(11); // Cyan
    // Display available versions.
    std::cout << "\nAvailable versions:" << std::endl;
    setConsoleColor(7);
    
    for (size_t i = 0; i < folders.size(); i++) {
        std::cout << (i + 1) << ". " << folders[i] << std::endl;
    }
    std::cout << std::endl;
    
    // Get user selection
    int versionChoice;
    do {
        std::cout << "Select version (1-" << folders.size() << "): ";
        std::cin >> versionChoice;
    } while (versionChoice < 1 || versionChoice > static_cast<int>(folders.size()));
    
    std::string selectedFolder = dataOrksPath + "\\" + folders[versionChoice - 1];
    std::string selectedName = folders[versionChoice - 1];
    
    setConsoleColor(14); // Yellow
    // Confirm action
    std::cout << "\nThis action will delete all DataX.ork (where X is a number from 1 to 9) in the game folder." << std::endl;
    setConsoleColor(7);
    
    std::cout << "Do you wish to continue? (y/n): ";
    char confirm;
    std::cin >> confirm;
    
    if (confirm == 'y' || confirm == 'Y') {
        try {
            removeDataFiles();
            copyDataFiles(selectedFolder, selectedName);
        }
        catch (const std::exception& e) {
            setConsoleColor(12); // Red
            std::cerr << "Critical error while trying to remove or copy data files: " << e.what() << std::endl;
            setConsoleColor(7);
            std::cin.ignore();
            std::cout << "Exiting...";
            std::cin.get();
            return;
        }

        setConsoleColor(10); // Green
        std::cout << "\nVersion successfully changed!" << std::endl;
        setConsoleColor(7);
        std::cin.ignore();
        std::cout << "Press Enter to continue...";
        std::cin.get();
    }
    else {
        setConsoleColor(14); // Yellow
        std::cout << "Operation cancelled." << std::endl;
        setConsoleColor(7);
        std::cin.ignore();
        std::cout << "Press Enter to continue...";
        std::cin.get();
    }
}

// Launch game executable.
void launchGame() {
    std::string gameExePath = getFullPath(gameExecutable);
    
    if (!fs::exists(gameExePath)) {
        setConsoleColor(12); // Red
        std::cout << "Error: Game executable not found: " << gameExePath << std::endl;
        setConsoleColor(7);
        std::cin.ignore();
        std::cout << "Press Enter to continue...";
        std::cin.get();
        return;
    }
    
    setConsoleColor(11); // Cyan
    std::cout << "Launching game: " << gameExecutable << std::endl;
    setConsoleColor(7);
    
    // Use ShellExecute to launch the game.
    HINSTANCE result = ShellExecuteA(
        NULL,
        "open",
        gameExePath.c_str(),
        NULL,
        gamePath.c_str(),
        SW_SHOWNORMAL
    );
    
    if ((INT_PTR)result <= 32) {
        setConsoleColor(12); // Red
        std::cout << "Error: Failed to launch game!" << std::endl;
        setConsoleColor(7);
    } else {
        setConsoleColor(10); // Green
        std::cout << "Game launched successfully!" << std::endl;
        setConsoleColor(7);
    }
    
    std::cin.ignore();
    std::cout << "Press Enter to continue...";
    std::cin.get();
}

// Launch game with 5x SpeedHack.
void launchGameWithSpeedHack() {
    if (DevToolsExigoSpeedhackInitiatorExecutableName.empty()) {
        setConsoleColor(12); // Red
        std::cout << "Error: DevToolsExigoSpeedhackInitiatorExecutableName not configured!" << std::endl;
        setConsoleColor(7);
        std::cin.ignore();
        std::cout << "Press Enter to continue...";
        std::cin.get();
        return;
    }
    
    std::string speedHackExePath = getFullPath(DevToolsExigoSpeedhackInitiatorExecutableName);
    
    if (!fs::exists(speedHackExePath)) {
        setConsoleColor(12); // Red
        std::cout << "Error: SpeedHack executable not found: " << speedHackExePath << std::endl;
        setConsoleColor(7);
        std::cin.ignore();
        std::cout << "Press Enter to continue...";
        std::cin.get();
        return;
    }
    
    setConsoleColor(11); // Cyan
    std::cout << "Launching game with " << devToolsExigoSpeedhackValue << "x SpeedHack: " << DevToolsExigoSpeedhackInitiatorExecutableName << std::endl;
    setConsoleColor(7);

    // Prepare argument with speedhack value.
    std::string speedHackArgs = std::to_string(devToolsExigoSpeedhackValue);
    
    // Use ShellExecute to launch the game with speedhack.
    HINSTANCE result = ShellExecuteA(
        NULL,
        "open",
        speedHackExePath.c_str(),
        speedHackArgs.c_str(),  // Argument with speedhack value.
        gamePath.c_str(),
        SW_SHOWNORMAL
    );
    
    if ((INT_PTR)result <= 32) {
        setConsoleColor(12); // Red
        std::cout << "Error: Failed to launch game with SpeedHack!" << std::endl;
        setConsoleColor(7);
    } else {
        setConsoleColor(10); // Green
        std::cout << "Game with SpeedHack launched successfully!" << std::endl;
        setConsoleColor(7);
    }
    
    std::cin.ignore();
    std::cout << "Press Enter to continue...";
    std::cin.get();
}

// Build DataX.ork AND launch game with SpeedHack.
void buildAndLaunchWithSpeedHack() {
    setConsoleColor(11); // Cyan
    std::cout << "=== Build and Launch ===" << std::endl;
    setConsoleColor(7);
    std::cout << std::endl;
    
    // Step 1: Build DataX.ork
    if (devToolsORKCmpPath.empty() || devToolsORKBuildSourcePath.empty() || 
        devToolsORKBuildTargetPath.empty() || devToolsORKBuildFileListPath.empty()) {
        setConsoleColor(12); // Red
        std::cout << "Error: ORK build paths not configured!" << std::endl;
        setConsoleColor(7);
        std::cin.ignore();
        std::cout << "Press Enter to continue...";
        std::cin.get();
        return;
    }
    
    std::string orkcmpPath = devToolsORKCmpPath + "\\orkcmp.exe";
    
    if (!fs::exists(orkcmpPath)) {
        setConsoleColor(12); // Red
        std::cout << "Error: orkcmp.exe not found at: " << orkcmpPath << std::endl;
        setConsoleColor(7);
        std::cin.ignore();
        std::cout << "Press Enter to continue...";
        std::cin.get();
        return;
    }
    
    std::string command = "\"\"" + orkcmpPath + "\" -g AOX -o -d \"" + 
                         devToolsORKBuildSourcePath + "\" \"" + 
                         devToolsORKBuildTargetPath + "\" \"" + 
                         devToolsORKBuildFileListPath + "\"\"";
    
    setConsoleColor(14); // Yellow
    std::cout << "Step 1/2: Building DataX.ork..." << std::endl;
    setConsoleColor(7);
    
    int buildResult = system(command.c_str());
    
    if (buildResult != 0) {
        setConsoleColor(12); // Red
        std::cout << "\nError: Build failed with code " << buildResult << std::endl;
        std::cout << "Cannot launch game - build must succeed first!" << std::endl;
        setConsoleColor(7);
        std::cin.ignore();
        std::cout << "Press Enter to continue...";
        std::cin.get();
        return;
    }
    
    setConsoleColor(10); // Green
    std::cout << "\nBuild completed successfully!" << std::endl;
    setConsoleColor(7);
    std::cout << std::endl;
    
    // Step 2: Launch game with SpeedHack.
    if (DevToolsExigoSpeedhackInitiatorExecutableName.empty()) {
        setConsoleColor(12); // Red
        std::cout << "Error: DevToolsExigoSpeedhackInitiatorExecutableName not configured!" << std::endl;
        setConsoleColor(7);
        std::cin.ignore();
        std::cout << "Press Enter to continue...";
        std::cin.get();
        return;
    }
    
    std::string speedHackExePath = getFullPath(DevToolsExigoSpeedhackInitiatorExecutableName);
    
    if (!fs::exists(speedHackExePath)) {
        setConsoleColor(12); // Red
        std::cout << "Error: SpeedHack executable not found: " << speedHackExePath << std::endl;
        setConsoleColor(7);
        std::cin.ignore();
        std::cout << "Press Enter to continue...";
        std::cin.get();
        return;
    }
    
    setConsoleColor(14); // Yellow
    std::cout << "Step 2/2: Launching game with " << devToolsExigoSpeedhackValue << "x SpeedHack..." << std::endl;
    setConsoleColor(7);

    // Prepare argument with speedhack value.
    std::string speedHackArgs = std::to_string(devToolsExigoSpeedhackValue);
    
    HINSTANCE result = ShellExecuteA(
        NULL,
        "open",
        speedHackExePath.c_str(),
        speedHackArgs.c_str(),  // Argument with speedhack value.
        gamePath.c_str(),
        SW_SHOWNORMAL
    );
    
    if ((INT_PTR)result <= 32) {
        setConsoleColor(12); // Red
        std::cout << "Error: Failed to launch game with SpeedHack!" << std::endl;
        setConsoleColor(7);
    } else {
        setConsoleColor(10); // Green
        std::cout << "\nBuild and launch completed successfully!" << std::endl;
        setConsoleColor(7);
    }
    
    std::cin.ignore();
    std::cout << "Press Enter to continue...";
    std::cin.get();
}

// Build DataX.ork using orkcmp tool.
void buildDataORK() {
    // Verify all required paths are configured.
    if (devToolsORKCmpPath.empty()) {
        setConsoleColor(12); // Red
        std::cout << "Error: DevToolsORKCmpPath not configured!" << std::endl;
        setConsoleColor(7);
        std::cin.ignore();
        std::cout << "Press Enter to continue...";
        std::cin.get();
        return;
    }
    
    if (devToolsORKBuildSourcePath.empty()) {
        setConsoleColor(12); // Red
        std::cout << "Error: DevToolsORKBuildSourcePath not configured!" << std::endl;
        setConsoleColor(7);
        std::cin.ignore();
        std::cout << "Press Enter to continue...";
        std::cin.get();
        return;
    }
    
    if (devToolsORKBuildTargetPath.empty()) {
        setConsoleColor(12); // Red
        std::cout << "Error: DevToolsORKBuildTargetPath not configured!" << std::endl;
        setConsoleColor(7);
        std::cin.ignore();
        std::cout << "Press Enter to continue...";
        std::cin.get();
        return;
    }
    
    if (devToolsORKBuildFileListPath.empty()) {
        setConsoleColor(12); // Red
        std::cout << "Error: DevToolsORKBuildFileListPath not configured!" << std::endl;
        setConsoleColor(7);
        std::cin.ignore();
        std::cout << "Press Enter to continue...";
        std::cin.get();
        return;
    }
    
    // Build orkcmp.exe path.
    std::string orkcmpPath = devToolsORKCmpPath + "\\orkcmp.exe";
    
    if (!fs::exists(orkcmpPath)) {
        setConsoleColor(12); // Red
        std::cout << "Error: orkcmp.exe not found at: " << orkcmpPath << std::endl;
        setConsoleColor(7);
        std::cin.ignore();
        std::cout << "Press Enter to continue...";
        std::cin.get();
        return;
    }
    
    // Build command line with proper escaping for paths with spaces
    std::string command = "\"\"" + orkcmpPath + "\" -g AOX -o -d \"" + 
                         devToolsORKBuildSourcePath + "\" \"" + 
                         devToolsORKBuildTargetPath + "\" \"" + 
                         devToolsORKBuildFileListPath + "\"\"";
    
    setConsoleColor(11); // Cyan
    std::cout << "Building DataX.ork..." << std::endl;
    std::cout << "Executing orkcmp..." << std::endl;
    setConsoleColor(7);
    std::cout << std::endl;
    
    // Execute command.
    int result = system(command.c_str());
    
    std::cout << std::endl;
    if (result == 0) {
        setConsoleColor(10); // Green
        std::cout << "DataX.ork built successfully!" << std::endl;
        setConsoleColor(7);
    } else {
        setConsoleColor(12); // Red
        std::cout << "Error: Build failed with code " << result << std::endl;
        setConsoleColor(7);
    }
    
    std::cin.ignore();
    std::cout << "Press Enter to continue...";
    std::cin.get();
}

// Launch editor executable.
void launchEditor() {
    if (editorExecutable.empty()) {
        setConsoleColor(12); // Red
        std::cout << "Error: Editor executable not configured in ExigoHelper.conf!" << std::endl;
        setConsoleColor(7);
        std::cin.ignore();
        std::cout << "Press Enter to continue...";
        std::cin.get();
        return;
    }
    
    std::string editorExePath = getFullPath(editorExecutable);
    
    if (!fs::exists(editorExePath)) {
        setConsoleColor(12); // Red
        std::cout << "Error: Editor executable not found: " << editorExePath << std::endl;
        setConsoleColor(7);
        std::cin.ignore();
        std::cout << "Press Enter to continue...";
        std::cin.get();
        return;
    }
    
    setConsoleColor(11); // Cyan
    std::cout << "Launching editor: " << editorExecutable << std::endl;
    setConsoleColor(7);
    
    // Use ShellExecute to launch the editor.
    HINSTANCE result = ShellExecuteA(
        NULL,
        "open",
        editorExePath.c_str(),
        NULL,
        gamePath.c_str(),
        SW_SHOWNORMAL
    );
    
    if ((INT_PTR)result <= 32) {
        setConsoleColor(12); // Red
        std::cout << "Error: Failed to launch editor!" << std::endl;
        setConsoleColor(7);
    } else {
        setConsoleColor(10); // Green
        std::cout << "Editor launched successfully!" << std::endl;
        setConsoleColor(7);
    }
    
    std::cin.ignore();
    std::cout << "Press Enter to continue...";
    std::cin.get();
}
void handleDgVoodoo() {
    std::string dgVoodooFile = getFullPath("dgVoodoo.conf");
    
    if (!fs::exists(dgVoodooFile)) {
        setConsoleColor(12); // Red
        std::cout << "Error: dgVoodoo.conf file does not exist!" << std::endl;
        setConsoleColor(7);
        std::cin.ignore();
        std::cout << "Press Enter to continue...";
        std::cin.get();
        return;
    }
    
    setConsoleColor(11); // Cyan
    std::cout << "\nEnable or disable?" << std::endl;
    setConsoleColor(7);
    std::cout << "1 - Enable" << std::endl;
    std::cout << "2 - Disable" << std::endl;
    
    int choice;
    std::cout << "\nChoose option (1 or 2): ";
    std::cin >> choice;
    
    std::ifstream fileIn(dgVoodooFile);
    std::string content((std::istreambuf_iterator<char>(fileIn)),
                        std::istreambuf_iterator<char>());
    fileIn.close();
    
    if (content.find("DisableAndPassThru") == std::string::npos) {
        setConsoleColor(12); // Red
        std::cout << "Error: 'DisableAndPassThru' entry not found in file!" << std::endl;
        setConsoleColor(7);
        std::cin.ignore();
        std::cout << "Press Enter to continue...";
        std::cin.get();
        return;
    }
    
    std::string newContent;
    if (choice == 1) {
        setConsoleColor(10); // Green
        std::cout << "\nEnabling... Setting DisableAndPassThru = false" << std::endl;
        setConsoleColor(7);
        
        size_t pos = 0;
        while ((pos = content.find("DisableAndPassThru", pos)) != std::string::npos) {
            size_t lineEnd = content.find('\n', pos);
            size_t equalPos = content.find('=', pos);
            if (equalPos != std::string::npos && equalPos < lineEnd) {
                size_t valueStart = content.find_first_not_of(" \t", equalPos + 1);
                if (valueStart != std::string::npos && content.substr(valueStart, 4) == "true") {
                    content.replace(valueStart, 4, "false");
                }
            }
            pos = lineEnd;
        }
        newContent = content;
    }
    else if (choice == 2) {
        setConsoleColor(14); // Yellow
        std::cout << "\nDisabling... Setting DisableAndPassThru = true" << std::endl;
        setConsoleColor(7);
        
        size_t pos = 0;
        while ((pos = content.find("DisableAndPassThru", pos)) != std::string::npos) {
            size_t lineEnd = content.find('\n', pos);
            size_t equalPos = content.find('=', pos);
            if (equalPos != std::string::npos && equalPos < lineEnd) {
                size_t valueStart = content.find_first_not_of(" \t", equalPos + 1);
                if (valueStart != std::string::npos && content.substr(valueStart, 5) == "false") {
                    content.replace(valueStart, 5, "true");
                }
            }
            pos = lineEnd;
        }
        newContent = content;
    }
    else {
        setConsoleColor(12); // Red
        std::cout << "Wrong choice!" << std::endl;
        setConsoleColor(7);
        return;
    }

    std::ofstream fileOut(dgVoodooFile);
    fileOut << newContent;
    fileOut.close();
    
    setConsoleColor(10); // Green
    std::cout << "Ready! File was updated." << std::endl;
    setConsoleColor(7);
    std::cin.ignore();
    std::cout << "Press Enter to continue...";
    std::cin.get();
}

// Launch dgVoodoo configurator.
void launchDgVoodoo() {
    std::string dgVoodooExe = "dgVoodooCpl.exe";
    std::string dgVoodooPath = getFullPath(dgVoodooExe);
    
    if (!fs::exists(dgVoodooPath)) {
        setConsoleColor(12); // Red
        std::cout << "Error: dgVoodoo configurator not found: " << dgVoodooPath << std::endl;
        std::cout << "Please install dgVoodoo in the game folder first!" << std::endl;
        setConsoleColor(7);
        std::cin.ignore();
        std::cout << "Press Enter to continue...";
        std::cin.get();
        return;
    }
    
    setConsoleColor(11); // Cyan
    std::cout << "Launching dgVoodoo configurator..." << std::endl;
    setConsoleColor(7);
    
    // Use ShellExecute to launch dgVoodoo configurator.
    HINSTANCE result = ShellExecuteA(
        NULL,
        "open",
        dgVoodooPath.c_str(),
        NULL,
        gamePath.c_str(),
        SW_SHOWNORMAL
    );
    
    if ((INT_PTR)result <= 32) {
        setConsoleColor(12); // Red
        std::cout << "Error: Failed to launch dgVoodoo configurator!" << std::endl;
        setConsoleColor(7);
    } else {
        setConsoleColor(10); // Green
        std::cout << "dgVoodoo configurator launched successfully!" << std::endl;
        setConsoleColor(7);
    }
    
    std::cin.ignore();
    std::cout << "Press Enter to continue...";
    std::cin.get();
}

int main() {
    // Load and verify game path.
    bool validSetup = loadGamePath();

    // Load DevTools configuration.
    loadDevToolsConfig();
    
    std::cout << std::endl;
    
    if (!validSetup) {
        setConsoleColor(12);
        std::cout << "\n========================================" << std::endl;
        std::cout << "CRITICAL ERROR: Invalid configuration!" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "\nPlease fix the following issues:" << std::endl;
        std::cout << "1. Make sure ExigoHelper.conf exists and contains valid ExigoSystemPath" << std::endl;
        std::cout << "2. Verify that game executable exists in the specified path" << std::endl;
        std::cout << "3. Verify that editor executable exists (if specified)" << std::endl;
        std::cout << "4. Make sure DataORKs folder exists in the game directory" << std::endl;
        setConsoleColor(7);
        std::cout << "\nProgram cannot continue with invalid configuration." << std::endl;
        std::cout << std::endl;
        system("pause");
        return 1;
    }
    
    setConsoleColor(10);
    std::cout << "\nConfiguration validated successfully!" << std::endl;
    setConsoleColor(7);
    std::cout << std::endl;
    system("pause");
    
    while (true) {
        system("cls");
        
        setConsoleColor(11); // Cyan
        std::cout << "=== Armies of Exigo Game Helper (Version: "<< appVersion << ") ===" << std::endl;
        setConsoleColor(7);
        std::cout << std::endl;
        
        showCurrentVersion();
        std::cout << std::endl;
        
        std::cout << "Select action:" << std::endl;
        std::cout << "1. Launch game" << std::endl;
        std::cout << "2. Launch editor" << std::endl;
        std::cout << "3. Change data version of the game" << std::endl;
        std::cout << "4. Launch dgVoodoo configurator" << std::endl;
        std::cout << "5. Enable/disable dgVoodoo" << std::endl;

        // Show DevTools option only if enabled.
        if (devToolsEnabled) {
            std::cout << "6. Build DataX.ork" << std::endl;
            std::cout << "7. Run game with " << devToolsExigoSpeedhackValue << "x SpeedHack" << std::endl;
            std::cout << "8. Build DataX.ork AND Run game with " << devToolsExigoSpeedhackValue << "x SpeedHack" << std::endl;
            std::cout << "9. Change SpeedHack value" << std::endl;
        }
        std::cout << "0. Exit" << std::endl;
        std::cout << std::endl;
        
        int choice;
        std::cout << "Enter your choice: ";
        std::cin >> choice;
        
        if (choice == 1) {
            launchGame();
        }
        else if (choice == 2) {
            launchEditor();
        }
        else if (choice == 3) {
            handleVersionChange();
        }
        else if (choice == 4) {
            launchDgVoodoo();
        }
        else if (choice == 5) {
            handleDgVoodoo();
        }
        else if (choice == 6 && devToolsEnabled) {
            buildDataORK();
        }
        else if (choice == 7 && devToolsEnabled) {
            launchGameWithSpeedHack();
        }
        else if (choice == 8 && devToolsEnabled) {
            buildAndLaunchWithSpeedHack();
        }
        else if (choice == 9 && devToolsEnabled) {
            changeSpeedHackValue();
        }
        else if (choice == 0) {
            setConsoleColor(10); // Green
            std::cout << "Goodbye!" << std::endl;
            setConsoleColor(7);
            break;
        }
        else {
            setConsoleColor(12); // Red
            std::cout << "Invalid choice. Please try again." << std::endl;
            setConsoleColor(7);
            std::cin.ignore();
            std::cout << "Press Enter to continue...";
            std::cin.get();
        }
    }
    
    return 0;
}