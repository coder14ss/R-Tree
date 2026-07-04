#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <limits> 

#define MAX_CHILDREN 4

using namespace std;

// Structure representing a spatial bounding box
struct BoundingBox {
    float xmin, ymin, xmax, ymax;

    // Default constructor creates a "reversed" box so any expansion overrides it
    BoundingBox() {
        xmin = numeric_limits<float>::max();
        ymin = numeric_limits<float>::max();
        xmax = -numeric_limits<float>::max();
        ymax = -numeric_limits<float>::max();
    }

    BoundingBox(float x_min, float y_min, float x_max, float y_max)
        : xmin(x_min), ymin(y_min), xmax(x_max), ymax(y_max) {}

    void expandToInclude(const BoundingBox& other) {
        xmin = min(xmin, other.xmin);
        ymin = min(ymin, other.ymin);
        xmax = max(xmax, other.xmax);
        ymax = max(ymax, other.ymax);
    }
    
    bool intersects(const BoundingBox& other) const {
        return (xmin <= other.xmax && xmax >= other.xmin &&
                ymin <= other.ymax && ymax >= other.ymin);
    }
};

// Structure for each multimedia item
struct MultimediaContent {
    int id;
    string title;
    string tags;
    BoundingBox box;

    MultimediaContent(int content_id, const string& content_title, const string& content_tags, BoundingBox content_box)
        : id(content_id), title(content_title), tags(content_tags), box(content_box) {}
};

// Structure for an R-Tree node
struct RTreeNode {
    bool is_leaf;
    BoundingBox box;
    vector<MultimediaContent*> contents;
    vector<RTreeNode*> children;

    RTreeNode(bool leaf, BoundingBox bounding_box)
        : is_leaf(leaf), box(bounding_box) {}

    ~RTreeNode() {
        for (auto content : contents) { delete content; }
        for (auto child : children) { delete child; }
    }

    void addContent(MultimediaContent* content) {
        contents.push_back(content);
        box.expandToInclude(content->box);
    }

    bool needsSplit() const {
        return is_leaf ? (contents.size() > MAX_CHILDREN) : (children.size() > MAX_CHILDREN);
    }

    int deleteContent(const BoundingBox& query_box) {
        int count = 0;
        if (is_leaf) {
            auto it = contents.begin();
            while (it != contents.end()) {
                // Delete if it perfectly matches the query box
                if ((*it)->box.xmin == query_box.xmin && (*it)->box.ymin == query_box.ymin &&
                    (*it)->box.xmax == query_box.xmax && (*it)->box.ymax == query_box.ymax) {
                    delete *it; 
                    it = contents.erase(it); 
                    count++;
                } else {
                    ++it;
                }
            }
            box = BoundingBox();
            for (auto* content : contents) { box.expandToInclude(content->box); }
        } else {
            for (auto* child : children) {
                if (child->box.intersects(query_box)) {
                    count += child->deleteContent(query_box);
                }
            }
            box = BoundingBox();
            for (auto* child : children) { box.expandToInclude(child->box); }
        }
        return count;
    }
};

// Class for the R-Tree
class RTree {
public:
    RTree() { root = new RTreeNode(true, BoundingBox()); }
    ~RTree() { delete root; }

    void insert(MultimediaContent* content) {
        insert(root, content);
        if (root->needsSplit()) { splitRoot(); }
    }

    int search(const BoundingBox& query_box, ofstream& output_file) {
        return searchNode(root, query_box, output_file);
    }

    int deleteContent(const BoundingBox& query_box) {
        return root->deleteContent(query_box);
    }

    vector<MultimediaContent*> getAllContents() {
        vector<MultimediaContent*> all_contents;
        gatherContents(root, all_contents);
        return all_contents;
    }

private:
    RTreeNode* root;

    void gatherContents(RTreeNode* node, vector<MultimediaContent*>& all_contents) {
        if (node->is_leaf) {
            all_contents.insert(all_contents.end(), node->contents.begin(), node->contents.end());
        } else {
            for (auto* child : node->children) { gatherContents(child, all_contents); }
        }
    }

    int searchNode(RTreeNode* node, const BoundingBox& query_box, ofstream& output_file) {
        int count = 0;
        if (node->box.intersects(query_box)) {
            if (node->is_leaf) {
                for (auto* content : node->contents) {
                    // Check for exact match
                    if (content->box.xmin == query_box.xmin && content->box.ymin == query_box.ymin &&
                        content->box.xmax == query_box.xmax && content->box.ymax == query_box.ymax) {
                        printContent(content, output_file);
                        count++;
                    }
                }
            } else {
                for (auto* child : node->children) {
                    count += searchNode(child, query_box, output_file);
                }
            }
        }
        return count;
    }

    void printContent(const MultimediaContent* content, ofstream& output_file) {
        output_file << "ID: " << content->id << ", Title: " << content->title
                    << ", Tags: " << content->tags
                    << ", Bounding Box: [" << content->box.xmin << ", "
                    << content->box.ymin << ", " << content->box.xmax << ", "
                    << content->box.ymax << "]\n";
    }

    void insert(RTreeNode* node, MultimediaContent* content) {
        if (node->is_leaf) {
            node->addContent(content);
        } else {
            RTreeNode* best_child = nullptr;
            for (auto* child : node->children) {
                if (!best_child || areaExpansion(child->box, content->box) < areaExpansion(best_child->box, content->box)) {
                    best_child = child;
                }
            }
            insert(best_child, content);
            node->box.expandToInclude(best_child->box);
            
            if (best_child->needsSplit()) {
                RTreeNode* new_child = splitNode(best_child);
                node->children.push_back(new_child);
                node->box.expandToInclude(new_child->box);
            }
        }
    }

    void splitRoot() {
        RTreeNode* new_root = new RTreeNode(false, BoundingBox());
        new_root->children.push_back(root);
        new_root->box.expandToInclude(root->box);
        
        RTreeNode* split_child = splitNode(root);
        new_root->children.push_back(split_child);
        new_root->box.expandToInclude(split_child->box);
        
        root = new_root;
    }

    RTreeNode* splitNode(RTreeNode* node) {
        RTreeNode* new_node = new RTreeNode(node->is_leaf, BoundingBox());
        
        if (node->is_leaf) {
            for (size_t i = MAX_CHILDREN / 2; i < node->contents.size(); ++i) {
                new_node->addContent(node->contents[i]);
            }
            node->contents.resize(MAX_CHILDREN / 2);
            
            node->box = BoundingBox();
            for (auto* content : node->contents) { node->box.expandToInclude(content->box); }
        } else {
            for (size_t i = MAX_CHILDREN / 2; i < node->children.size(); ++i) {
                new_node->children.push_back(node->children[i]);
                new_node->box.expandToInclude(node->children[i]->box);
            }
            node->children.resize(MAX_CHILDREN / 2);
            
            node->box = BoundingBox();
            for (auto* child : node->children) { node->box.expandToInclude(child->box); }
        }
        return new_node;
    }

    float areaExpansion(const BoundingBox& box, const BoundingBox& new_box) {
        float expanded_xmin = min(box.xmin, new_box.xmin);
        float expanded_ymin = min(box.ymin, new_box.ymin);
        float expanded_xmax = max(box.xmax, new_box.xmax);
        float expanded_ymax = max(box.ymax, new_box.ymax);
        
        float current_area = (box.xmax - box.xmin) * (box.ymax - box.ymin);
        if (current_area < 0) current_area = 0; 
        
        float new_area = (expanded_xmax - expanded_xmin) * (expanded_ymax - expanded_ymin);
        return new_area - current_area;
    }
};

unordered_map<string, RTree*> loadUsersFromCSV(const string& filename) {
    unordered_map<string, RTree*> users;
    ifstream user_file(filename);
    if (!user_file) return users;

    string line;
    while (getline(user_file, line)) {
        stringstream ss(line);
        string username, title, tags;
        int id;
        float xmin, ymin, xmax, ymax;

        getline(ss, username, ',');
        ss >> id; ss.ignore();
        getline(ss, title, ',');
        getline(ss, tags, ',');
        ss >> xmin; ss.ignore();
        ss >> ymin; ss.ignore();
        ss >> xmax; ss.ignore();
        ss >> ymax;

        if (users.find(username) == users.end()) {
            users[username] = new RTree();
        }

        BoundingBox box(xmin, ymin, xmax, ymax);
        MultimediaContent* content = new MultimediaContent(id, title, tags, box);
        users[username]->insert(content);
    }
    user_file.close();
    return users;
}

void appendUserContentToCSV(const string& filename, const string& username, MultimediaContent* content) {
    ofstream user_file(filename, ios::app);
    if (user_file) {
        user_file << username << "," << content->id << "," << content->title 
                  << "," << content->tags << "," << content->box.xmin << "," 
                  << content->box.ymin << "," << content->box.xmax << "," 
                  << content->box.ymax << "\n";
    }
    user_file.close();
}

void rewriteCSV(const string& filename, const unordered_map<string, RTree*>& users) {
    ofstream user_file(filename);
    if (user_file) {
        for (const auto& pair : users) {
            const string& username = pair.first;
            RTree* tree = pair.second;
            vector<MultimediaContent*> contents = tree->getAllContents();
            for (const auto& content : contents) {
                user_file << username << "," << content->id << "," << content->title
                          << "," << content->tags << "," << content->box.xmin << ","
                          << content->box.ymin << "," << content->box.xmax << ","
                          << content->box.ymax << "\n";
            }
        }
    }
}

bool isValidBox(float xmin, float ymin, float xmax, float ymax) {
    if (xmin >= xmax || ymin >= ymax) {
        cout << "Error: Invalid coordinates. Minimum values must be strictly less than maximum values." << endl;
        return false;
    }
    return true;
}

int main() {
    unordered_map<string, RTree*> users = loadUsersFromCSV("users_content.csv");

    string username;
    cout << "Enter your username (or type 'new' to create a new user): ";
    cin >> username;

    if (username == "new") {
        cout << "Enter new username: ";
        cin >> username;
        if (users.find(username) != users.end()) {
            cout << "Username already exists. Try logging in." << endl;
            for (auto& pair : users) { delete pair.second; }
            return 0;
        }
        users[username] = new RTree();
    } else if (users.find(username) == users.end()) {
        cout << "Username not found. Please try again." << endl;
        for (auto& pair : users) { delete pair.second; }
        return 0;
    }

    int option;
    while (true) {
        cout << "\nSelect an option:\n1. Add new content\n2. Search content by bounding box\n3. Delete content by bounding box\n4. Exit\n> ";
        cin >> option;

        if (option == 1) {
            int id; string title, tags; float xmin, ymin, xmax, ymax;

            cout << "Enter content ID: "; cin >> id; cin.ignore();
            cout << "Enter title: "; getline(cin, title);
            cout << "Enter tags (comma-separated): "; getline(cin, tags);
            cout << "Enter bounding box coordinates (xmin ymin xmax ymax): ";
            cin >> xmin >> ymin >> xmax >> ymax;

            if (!isValidBox(xmin, ymin, xmax, ymax)) continue;

            MultimediaContent* new_content = new MultimediaContent(id, title, tags, BoundingBox(xmin, ymin, xmax, ymax));
            users[username]->insert(new_content);
            appendUserContentToCSV("users_content.csv", username, new_content);
            cout << "Content added successfully." << endl;

        } else if (option == 2) {
            float xmin, ymin, xmax, ymax;
            cout << "Enter bounding box coordinates to search (xmin ymin xmax ymax): ";
            cin >> xmin >> ymin >> xmax >> ymax;
            
            if (!isValidBox(xmin, ymin, xmax, ymax)) continue;

            BoundingBox query_box(xmin, ymin, xmax, ymax);
            string filename = username + "_search_result.txt";
            ofstream output_file(filename);
            
            if (output_file) {
                int found = users[username]->search(query_box, output_file);
                output_file.close();
                if (found > 0) {
                    cout << "Found " << found << " item(s). Results saved to '" << filename << "'." << endl;
                } else {
                    cout << "No matching content found for that bounding box." << endl;
                }
            } else {
                cout << "Unable to open results file." << endl;
            }

        } else if (option == 3) {
            float xmin, ymin, xmax, ymax;
            cout << "Enter bounding box coordinates to delete (xmin ymin xmax ymax): ";
            cin >> xmin >> ymin >> xmax >> ymax;
            
            if (!isValidBox(xmin, ymin, xmax, ymax)) continue;

            BoundingBox delete_box(xmin, ymin, xmax, ymax);
            int deleted = users[username]->deleteContent(delete_box);
            
            if (deleted > 0) {
                rewriteCSV("users_content.csv", users);
                cout << "Successfully deleted " << deleted << " item(s)." << endl;
            } else {
                cout << "No content found at those exact coordinates to delete." << endl;
            }

        } else if (option == 4) {
            cout << "Exiting the program." << endl;
            break;
        } else {
            cout << "Invalid option." << endl;
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
    }

    for (auto& pair : users) { delete pair.second; }
    cout << "Goodbye!" << endl;
    return 0;
}