#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <set>       // For unique suggestions and unique incorrect words
#include <algorithm> // For min (though not directly used in this recursive variant)
#include <cctype>    // For tolower, isalpha

using namespace std;

// ANSI Color Codes
const string RED_COLOR   = "\033[31m";
const string RESET_COLOR = "\033[0m";

// --- Trie Node Structure ---
class node
{
public:
    node* children[26];
    bool end_of_word;

    node()
    {
        for (int i = 0; i < 26; ++i)
            children[i] = nullptr;
        end_of_word = false;
    }
};

// --- Trie Class with Edit-Distance Suggestions ---
class Trie
{
public:
    node* root;
    Trie()
    {
        root = new node();
    }

    void insert_word(const string& word)
    {
        if (word.empty()) return;
        node* current = root;
        for (char ch : word)
        {
            int idx = ch - 'a';
            if (idx < 0 || idx >= 26) continue;
            if (!current->children[idx])
                current->children[idx] = new node();
            current = current->children[idx];
        }
        current->end_of_word = true;
    }

    bool search_word(const string& normalized)
    {
        if (normalized.empty()) return false;
        node* current = root;
        for (char ch : normalized)
        {
            int idx = ch - 'a';
            if (idx < 0 || idx >= 26 || !current->children[idx])
                return false;
            current = current->children[idx];
        }
        return current->end_of_word;
    }

    set<string> getEditDistanceSuggestions(const string& word, int maxDist)
    {
        set<string> suggestions;
        findRec(root, word, 0, "", 0, maxDist, suggestions);
        return suggestions;
    }

    ~Trie()
    {
        destroy(root);
    }

private:
    void findRec(node* cur,
                 const string& w,
                 size_t i,
                 string path,
                 int edits,
                 int maxEdits,
                 set<string>& out)
    {
        if (edits > maxEdits) return;
        if (i == w.size())
        {
            if (cur->end_of_word)
                out.insert(path);
            if (edits < maxEdits)
            {
                for (int k = 0; k < 26; ++k)
                {
                    if (cur->children[k])
                        findRec(cur->children[k], w, i, path + char('a' + k), edits + 1, maxEdits, out);
                }
            }
            return;
        }
        if (cur->end_of_word)
        {
            int rem = w.size() - i;
            if (edits + rem <= maxEdits)
                out.insert(path);
        }

        char c = w[i];
        // match or substitute
        for (int k = 0; k < 26; ++k)
        {
            if (cur->children[k])
            {
                char tc = 'a' + k;
                int cost = (tc == c ? 0 : 1);
                if (edits + cost <= maxEdits)
                    findRec(cur->children[k], w, i + 1, path + tc, edits + cost, maxEdits, out);
            }
        }

        if (edits < maxEdits)
        {
            // deletion
            findRec(cur, w, i + 1, path, edits + 1, maxEdits, out);
            // insertion
            for (int k = 0; k < 26; ++k)
            {
                if (cur->children[k])
                    findRec(cur->children[k], w, i, path + char('a' + k), edits + 1, maxEdits, out);
            }
        }
    }

    void destroy(node* cur)
    {
        if (!cur) return;
        for (int i = 0; i < 26; ++i)
            destroy(cur->children[i]);
        delete cur;
    }
};

// Normalize string: lowercase and strip non-alpha
string normalize(const string& s)
{
    string out;
    for (char c : s)
    {
        if (isalpha(c))
            out += tolower(c);
    }
    return out;
}

int main()
{
    Trie dictionaryTrie;

    // Load dictionary from file
    ifstream dictFile("dictionary_111.txt");
    if (!dictFile)
    {
        cerr << "Error opening dictionary_111.txt" << endl;
        return 1;
    }
    string w;
    while (dictFile >> w)
    {
        dictionaryTrie.insert_word(normalize(w));
    }
    dictFile.close();

    // Read input text
    ifstream inFile("input.txt");
    if (!inFile)
    {
        cerr << "Error opening input.txt" << endl;
        return 1;
    }

    set<string> incorrect_set;
    string colored_output;
    string plain_output;
    string buf;
    char ch;
    while (inFile.get(ch))
    {
        if (isalpha(ch))
        {
            buf += ch;
        }
        else
        {
            if (!buf.empty())
            {
                string norm = normalize(buf);
                bool wrong = !norm.empty() && !dictionaryTrie.search_word(norm);
                if (wrong)
                {
                    colored_output += RED_COLOR + buf + RESET_COLOR;
                    incorrect_set.insert(buf);
                }
                else
                {
                    colored_output += buf;
                }
                plain_output += buf;
                buf.clear();
            }
            colored_output += ch;
            plain_output += ch;
        }
    }
    if (!buf.empty())
    {
        string norm = normalize(buf);
        bool wrong = !norm.empty() && !dictionaryTrie.search_word(norm);
        if (wrong)
        {
            colored_output += RED_COLOR + buf + RESET_COLOR;
            incorrect_set.insert(buf);
        }
        else
        {
            colored_output += buf;
        }
        plain_output += buf;
    }
    inFile.close();

    // Print highlighted text to console
    cout << colored_output << endl;

    // Interactive suggestions
    if (!incorrect_set.empty())
    {
        const int maxDist = 1;
        for (const auto& orig : incorrect_set)
        {
            cout << "\nWrong word: " << orig << endl;
            auto sugg = dictionaryTrie.getEditDistanceSuggestions(normalize(orig), maxDist);
            vector<string> vs(sugg.begin(), sugg.end());
            cout << "Suggestions:\n";
            for (size_t i = 0; i < vs.size(); ++i)
            {
                cout << i << ". " << vs[i] << endl;
            }
            cout << "Enter index to use, or 'c' to custom, or 'i' to ignore: ";
            string choice;
            cin >> choice;
            string replacement = orig;
            if (choice == "c")
            {
                cin.ignore(1000, '\n');
                getline(cin, replacement);
            }
            else if (choice != "i")
            {
                int idx = stoi(choice);
                if (idx >= 0 && idx < static_cast<int>(vs.size()))
                    replacement = vs[idx];
            }

            // ── NEW: add chosen replacement into both trie and dictionary file ──
            if (choice != "i")
            {
                // 1) in-memory
                dictionaryTrie.insert_word(normalize(replacement));
                // 2) persistent
                ofstream dictAppend("dictionary_111.txt", ios::app);
                if (dictAppend)
                {
                    dictAppend << normalize(replacement) << '\n';
                }
            }

            // Replace in plain_output only
            size_t pos = 0;
            while ((pos = plain_output.find(orig, pos)) != string::npos)
            {
                plain_output.replace(pos, orig.length(), replacement);
                pos += replacement.length();
            }
        }
        // Save final plain text to output.txt (no escape codes)
        ofstream outFile("output.txt");
        outFile << plain_output;
        outFile.close();
        cout << "\nFinal corrected text written to output.txt" << endl;
    }
    else
    {
        cout << "\nNo misspellings found." << endl;
    }
    return 0;
}
