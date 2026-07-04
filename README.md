# 🗺️ Multimedia Content Library (R-Tree Spatial Database)

A console-based spatial database application written in C++ that allows users to store, search, and manage multimedia content (like photos or videos) using 2D coordinates. 

Instead of searching by text, this application uses a custom-built **R-Tree** data structure to perform lightning-fast spatial queries via Bounding Boxes.

## ✨ Features
* **Spatial Indexing:** Built from scratch using an R-Tree for highly efficient 2D spatial querying.
* **Dynamic Node Splitting:** Automatically balances the tree by splitting internal nodes and leaves when they reach maximum capacity (`MAX_CHILDREN = 4`).
* **CRUD Operations:** Add new content, search for overlapping content, and delete content perfectly by its spatial footprint.
* **Persistent Storage:** Automatically saves and loads the R-Tree data to a local `users_content.csv` file, ensuring no data is lost between sessions.
* **Multi-User Support:** Creates unique R-Trees and search logs for different usernames.

## 🧠 How the R-Tree Works
Every piece of multimedia content is assigned a **Bounding Box** defined by four coordinates: `[xmin, ymin, xmax, ymax]`. 

The R-Tree groups items that are close to each other into larger "parent" bounding boxes. When a user searches a specific area, the algorithm checks the massive parent boxes first. If the search query does not intersect with a parent box, the algorithm instantly ignores the entire branch, making the search much faster than a standard linear search.

## 🚀 Getting Started

### Prerequisites
To run this project, you will need a C++ compiler installed on your system:
* **Windows:** MinGW-w64 (GCC)
* **macOS:** Clang (via Xcode Command Line Tools)
* **Linux:** GCC (`build-essential`)

### Installation & Compilation
1. Clone this repository to your local machine:
   ```bash
   git clone [https://github.com/YOUR_USERNAME/YOUR_REPO_NAME.git](https://github.com/YOUR_USERNAME/YOUR_REPO_NAME.git)
   cd YOUR_REPO_NAME
