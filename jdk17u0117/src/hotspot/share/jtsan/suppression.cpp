#include "suppression.hpp"
#include "jtsanStack.hpp"

#include "memory/resourceArea.hpp"
#include "memory/allocation.hpp"

const char * def_top_frame_suppressions = "";

const char * def_frame_suppressions = 
    "java.lang.invoke.*\n"
    "java.util.concurrent.*\n";
    // "java.lang.ref.ReferenceQueue.*\n"
    // "java.lang.ref.Reference.*\n";

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
    char buffer[1024] = "";

    for (size_t i = 0, j = 0; ; i++) {
        if (suppression_string[i] == '\n' || suppression_string[i] == '\0') {
            if (j != 0) {
                buffer[j] = 0;
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

    JavaThread *thread = JavaThread::current();
    JTSanStack *stack = JavaThread::get_jtsan_stack(thread);
    uint64_t mask = 0xFFFFFFFFFFFF;

    // first check the top frame
    Method *mp = NULL;
    uint64_t raw_frame = stack->top();

    // first 48bits are the method pointer
    mp = (Method*)(raw_frame & mask);
    const char *fname = mp->external_name_as_fully_qualified();

    if (top_frame_suppressions->search(fname)) {
        return true;
    }

    size_t stack_size = stack->size();
    // now check the rest of the frames
    for (size_t i = stack_size - 1; i >= 0; i--) {
        raw_frame = stack->get(i);
        mp = (Method*)(raw_frame & mask);
        fname = mp->external_name_as_fully_qualified();
        if (frame_suppressions->search(fname)) {
            return true;
        }
    }

    // // first check the top frame
    // const char *frame = stack_trace->get_frame(0).method->external_name_as_fully_qualified();
    // if (top_frame_suppressions->search(frame)) {
    //     return true;
    // }

    // // now check the rest of the frames
    // for (size_t i = 0; i < stack_trace->frame_count(); i++) {
    //     frame = stack_trace->get_frame(i).method->external_name_as_fully_qualified();
    //     if (frame_suppressions->search(frame)) {
    //         return true;
    //     }
    // }

    return false;
}






