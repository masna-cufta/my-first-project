#include <iostream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <filesystem>
#include <vector>
#include <fstream>

using namespace std;

// Extract file paths enclosed in single quotes
vector<string> extract_files_1(const string &input) {
    vector<string> files;
    string file;
    bool inside_quotes = false;

    for (char c: input) {
        if (c == '\'') {
            inside_quotes = !inside_quotes;
            if (!inside_quotes && !file.empty()) {
                files.push_back(file);
                file.clear();
            }
        } else if (inside_quotes) {
            file += c;
        }
    }

    return files;
}

vector<string> extract_files_2(const string &input) {
    vector<string> files;
    string file;
    bool inside_quotes = false;

    for (char c: input) {
        if (c == '\"') {
            inside_quotes = !inside_quotes;
            if (!inside_quotes && !file.empty()) {
                files.push_back(file);
                file.clear();
            }
        } else if (inside_quotes) {
            file += c;
        }
    }

    return files;
}

string double_quotes(string input) {
    string output;
    bool inside_quotes = false;
    int count = 0;
    for (int i = 0; i < input.size(); i++) {
        char c = input[i];
        if (c == '\"' && (input[i - 1] == ' ' || input[i + 1] == ' ' || i == 0)) {
            inside_quotes = !inside_quotes;
        } else if (!inside_quotes && !output.empty() && count == 0) {
            output += " ";
            count++;
        } else if (c == '\\' && (input[i + 1] == '\\' || input[i + 1] == '$' || input[i + 1] == '\"')) {
            output += input[i + 1];
            i++;
            count = 0;
        } else if (c == '\"') {
            output += input[i + 1];
            i++;
            count = 0;
        } else if (inside_quotes) {
            output += c;
            count = 0;
        }
    }
    return output;
}

// Handle the `cat` command
void cat_command(const vector<string> &files) {
    for (const string &file: files) {
        ifstream file_stream(file);

        if (!file_stream.is_open()) {
            cerr << "Could not open file " << file << endl;
            continue;
        }

        // Print the file contents without spaces between files
        string line;
        while (getline(file_stream, line)) {
            cout << line;
        }

        file_stream.close();
    }
    cout << endl; // Add newline at the end
}

string remove_extra_spaces(const string &str) {
    stringstream ss(str);
    string word;
    string result;
    bool first = true;
    while (ss >> word) {
        if (!first) {
            result += " ";
        }
        result += word;
        first = false;
    }
    return result;
}

void curr_dir() {
    try {
        string cwd = filesystem::current_path().string();
        cout << cwd << endl;
    } catch (const filesystem::filesystem_error &e) {
        cerr << e.what() << endl;
    }
}

string get_path(string command) {
    string path_env = getenv("PATH");
    stringstream ss(path_env);
    string path;

    while (getline(ss, path, ':')) {
        string abs_path = path + "/" + command;

        if (filesystem::exists(abs_path)) {
            return abs_path;
        }
    }
    return "";
}

int main() {
    cout << "$ " << flush;

    string input;
    while (getline(cin, input)) {
        string command;
        vector<string> args;

        if (input[0] == '\'') {
            size_t pos = input.find('\'', 1);
            command = input.substr(1, pos - 1);
            stringstream ss(input.substr(pos + 1));
            string arg;
            while (ss >> arg) {
                args.push_back(arg);
            }
            cat_command(args);
            cout << "$ " << flush;
            continue;
        } else if (input[0] == '\"') {
            size_t pos = input.find('\"', 1);
            command = input.substr(1, pos - 1);
            stringstream ss(input.substr(pos + 1));
            string arg;
            while (ss >> arg) {
                args.push_back(arg);
            }
            cat_command(args);
            cout << "$ " << flush;
            continue;
        } else {
            stringstream ss(input);
            ss >> command;
            string arg;
            while (ss >> arg) {
                args.push_back(arg);
            }
        }

        if (command == "exit") {
            return 0;
        }
        if (command == "cd") {
            if (args.empty())
                cerr << "cd: command not found\n";
            else if (args[0] == "~") {
                try {
                    // Set path to home directory (replace with appropriate home path for your system).
                    filesystem::path home = filesystem::path(getenv("HOME") ? getenv("HOME") : getenv("USERPROFILE"));
                    if (!home.empty()) {
                        filesystem::current_path(home);
                    } else
                        cerr << "cd: Cannot determine the home directory\n";
                } catch (const filesystem::filesystem_error &e) {
                    cerr << "cd: " << e.what() << endl;
                }
            } else {
                try {
                    filesystem::current_path(args[0]);
                } catch (const filesystem::filesystem_error &e) {
                    cerr << "cd: " << args[0] << ": No such file or directory" << endl;
                }
            }
        } else if (command == "echo") {
            string out = input.substr(5);
            if (out[0] == '\'') {
                string out2 = out.substr(1, out.size() - 2);
                cout << out2 << endl;
            } else if (out[0] == '\"') {
                cout << double_quotes(out) << endl;
            } else {
                out = remove_extra_spaces(out);
                string out2;
                for (char c: out) {
                    if (c != '\\') {
                        out2 += c;
                    }
                }
                cout << out2 << endl;
            }
        } else if (command == "pwd") {
            curr_dir();
        } else if (command == "cat") {
            string out = input.substr(4);
            if (out[0] == '\'') {
                vector<string> files = extract_files_1(input);
                cat_command(files);
            } else if (out[0] == '\"') {
                vector<string> files = extract_files_2(input);
                cat_command(files);
            }
        } else if (command == "type") {
            string out = input.substr(5);
            if (out == "echo" || out == "exit" || out == "type" || out == "pwd" || out == "cd" || out == "ls") {
                cout << out << " is a shell builtin\n";
            } else {
                string path = get_path(out);
                if (path.empty()) {
                    cout << out << ": not found\n";
                } else {
                    cout << out << " is " << path << endl;
                }
            }
        } else if (command == "ls") {
            string path;
            if (args.empty()) {
                path = "."; // Use the current directory if no arguments are provided
            } else {
                path = args[0]; // Use the first argument as the directory path
            }
            try {
                for (const auto &entry: filesystem::directory_iterator(path)) {
                    cout << entry.path().filename().string() << endl; // Print only the filename
                }
            } catch (const filesystem::filesystem_error &e) {
                cerr << "ls: " << e.what() << endl; // Print error if directory cannot be accessed
            }
        } else {
            string path = get_path(command);
            if (path.empty()) {
                cerr << command << ": command not found\n";
            } else {
                string cmd_with_args = path;
                for (const string &arg: args) {
                    cmd_with_args += " " + arg;
                }

                int result = system(cmd_with_args.c_str());
                if (result != 0) {
                    cerr << "Error executing " << command << endl;
                }
            }
        }


        cout << "$ " << flush; // Ensure prompt is printed after any command
    }

    return 0;
}
