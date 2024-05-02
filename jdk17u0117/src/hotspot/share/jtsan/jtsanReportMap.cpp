#include "jtsanReportMap.hpp"

#include <cstring>
#include <cstdint>

JtsanReportMap* JtsanReportMap::instance = nullptr;

JtsanReportMap::JtsanReportMap() {
    _size = 0;
    _lock = new Mutex(Mutex::event, "JTSAN Report Map Lock");
    memset(_map, 0, sizeof(_map));
}

JtsanReportMap::~JtsanReportMap() {
    clear();
}

uint64_t fnv1a_64(void *bcp) {
    // address is 8 bytes
    uint64_t hash = 0xcbf29ce484222325;    
    for (int i = 0; i < 8; i++) {
        hash = hash ^ ((char*)bcp)[i];
        hash = hash * 0x100000001b3;
    }

    return hash % MAX_MAP_SIZE;
}

void JtsanReportMap::put(void* value) {
    uint64_t hash = fnv1a_64(value);
    hashmap* entry = new hashmap();
    entry->value = value;

    _lock->lock();
    entry->next = _map[hash];

    if (_size + 1 >= MAX_MAP_SIZE) {
        // for now clear the whole map
        clear();
    }

    _map[hash] = entry;
    _size++;

    _lock->unlock();
}

void* JtsanReportMap::get(void* value) {
    uint64_t hash = fnv1a_64(value);

    _lock->lock();

    hashmap* entry = _map[hash];
    void    *result = nullptr;

    while (entry != nullptr) {
        if (entry->value == value) {
            result = entry->value;
            break;
        }
        entry = entry->next;
    }

    _lock->unlock();
    return result;
}

void JtsanReportMap::clear() {
    _lock->lock();

    for (int i = 0; i < MAX_MAP_SIZE; i++) {
        hashmap* entry = _map[i];
        while (entry != nullptr) {
            hashmap* next = entry->next;
            delete entry;
            entry = next;
        }
        _map[i] = nullptr;
    }
    _size = 0;

    _lock->unlock();
}

JtsanReportMap* JtsanReportMap::get_instance() {
    if (instance == nullptr) {
        instance = new JtsanReportMap();
    }
    return instance;
}

void JtsanReportMap::jtsan_reportmap_destroy() {
    if (instance != nullptr) {
        delete instance;
        instance = nullptr;
    }
}

void JtsanReportMap::jtsan_reportmap_init() {
    if (instance == nullptr) {
        instance = new JtsanReportMap();
    }
}