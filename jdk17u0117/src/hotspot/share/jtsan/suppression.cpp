#include "suppression.hpp"

#include "memory/resourceArea.hpp"

const char * def_top_frame_suppressions = "";

const char * def_frame_suppressions = 
    "java.lang.invoke.*\n"
    "java.util.concurrent.locks.*\n";

Trie::Trie() {
    root = new TrieNode();
}

void Trie::insert(const char *name) {
    TrieNode* current = root;

    const char *str = name;
    for (char c = *str; c != '\0'; c = *(++str)) {
        if (c == '*') {
            current->has_wildcard = true;
            break;
        }

        if (current->children.find(c) == current->children.end()) {
            current->children[c] = new TrieNode();
        }
        current = current->children[c];
    }
    current->is_end_of_word = true;
}

bool Trie::search(const char *name) {
    TrieNode* current = root;

    // if root has just a wildcard and no other children then it matches everything aka "*"
    if (current->has_wildcard && current->children.size() == 0) {
        return true;
    }

    const char *str = name;
    for (char c = *str; c != '\0'; c = *(++str)) {
        //not a children of the current node
        if (current->children.find(c) == current->children.end()) {
            if (current->has_wildcard) {
                return true;
            }
            return false;
        }
        current = current->children[c];
    }

    return current->is_end_of_word;
}

static void add_suppressions(Trie *trie, const char *suppression_string) {
    // assume each string can have up to 1023 characters
    char buffer[1024] = {0};

    fprintf(stderr, "Suppression string: %s\n", suppression_string);

    for (size_t i = 0, j = 0; ; i++) {
        if (suppression_string[i] == '\n' || suppression_string[i] == '\0') {
            if (j != 0) {
                buffer[j] = 0;
                fprintf(stderr, "Adding suppression: %s\n", buffer);
                trie->insert(buffer);
                buffer[j = 0] = 0;
            }

            if (suppression_string[i] == '\0') {
                break;
            }
        } else {
            if (j >= 1023) {
                fprintf(stderr, "Suppression string too long\n");
                exit(1);
            }

            buffer[j++] = suppression_string[i];
        }
    }
}

Trie *JTSanSuppression::top_frame_suppressions = nullptr;
Trie *JTSanSuppression::frame_suppressions     = nullptr;

void JTSanSuppression::init(void) {
    top_frame_suppressions = new Trie();
    frame_suppressions     = new Trie();

    add_suppressions(top_frame_suppressions, def_top_frame_suppressions);
    add_suppressions(frame_suppressions    , def_frame_suppressions);
}

bool JTSanSuppression::is_suppressed(JTSanStackTrace *stack_trace) {
    ResourceMark rm;

    // first check the top frame
    const char *frame = stack_trace->get_frame(0).method->external_name_as_fully_qualified();
    fprintf(stderr, "Checking suppression for frame: %s -> ", frame);
    if (top_frame_suppressions->search(frame)) {
        fprintf(stderr, "suppressed\n");
        return true;
    }
    fprintf(stderr, "not suppressed\n");

    // now check the rest of the frames
    for (size_t i = 1; i < stack_trace->frame_count(); i++) {
        ResourceMark rm;
        frame = stack_trace->get_frame(i).method->external_name_as_fully_qualified();
        fprintf(stderr, "Checking suppression for frame: %s -> ", frame);
        if (frame_suppressions->search(frame)) {
            fprintf(stderr, "suppressed\n");
            return true;
        }

        fprintf(stderr, "not suppressed\n");
    }

    return false;
}






