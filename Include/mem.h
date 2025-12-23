#pragma once
#include <defines.h>

#ifndef MEMORY_H
#define MEMORY_H

/**
 * @enum memory_tag
 * @brief Tags used for tracking different types of allocated memory.
 *
 * Useful for memory management, debugging, and profiling allocations.
 */
typedef enum memory_tag {
    MEMTAG_VERTEX_BUFFER = 1,       /**< Memory allocated for vertex buffers */
    MEMTAG_ELEMENT_BUFFER,          /**< Memory allocated for element/index buffers */
    MEMTAG_VERTEX_ARRAY,            /**< Memory allocated for vertex array objects */
    MEMTAG_VERTEX_ARRAY_ELEMENT,    /**< Memory allocated for individual vertex array elements */
    MEMTAG_CANVAS,                  /**< Memory allocated for canvas structs */
    MEMTAG_COLOR,                   /**< Memory allocated for color buffers */
    MEMTAG_SHADER,                  /**< Memory allocated for shaders */
    MEMTAG_SPRITE,                  /**< Memory allocated for generic objects */
    MEMTAG_CAMERA,                  /**< Memory allocated for camera structs */
    MEMTAG_BYTE,                    /**< Memory allocated for raw byte buffers */
    MEMTAG_FRAME,                   /**< Memory allocated for frame/window structs */
    MEMTAG_TEXTURE,                 /**< Memory allocated for textures */
    MEMTAG_BUTTON,
    MEMTAG_COMPONENT_NODE,
    MEMTAG_POINTER,
    MEMTAG_PANEL,
    __MEMTAG_COUNT__                /**< Total number of memory tags */
} mem_tag;

/**
 * @brief Generic memory buffer descriptor
 */
typedef struct buffer {
    u64 size;       /**< Size of the memory in bytes */
    mem_tag tag;    /**< Tag identifying the type of memory */
    void* ptr;      /**< Pointer to the allocated memory */
} buf_t;

/**
 * @brief Allocates a new memory buffer
 * @param buffer Pointer to a buffer descriptor
 * @param zero If true, zero-initialize the allocated memory
 * @return true if allocation succeeded, false otherwise
 */
bool new_buf(buf_t* buffer, const bool zero);

/**
 * @brief Resizes an existing buffer
 * @param buffer Pointer to a buffer descriptor
 * @param new_size New size in bytes
 * @return true if reallocation succeeded, false otherwise
 */
bool renew_buf(buf_t* buffer, const u64 new_size);

/**
 * @brief Frees a memory buffer
 * @param buffer Pointer to a buffer descriptor
 */
void del_buf(buf_t* buffer);

/**
 * @brief Prints a memory usage table for all allocated buffers
 */
void print_memtable(void);

#endif //MEMORY_H
