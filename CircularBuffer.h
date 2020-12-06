//
//  CircularBuffer.h
//  CircularBuffer
//
//  Created by Daniel Pietz on 11/5/20.
//  This class provides declaration and funtionality for the
//  Circular Buffer class. This class allows for temporary
//  storage of large amounts of data, and will overwrite
//  old data when the buffer becomes fill.
//

#ifndef CircularBuffer_h
#define CircularBuffer_h
#ifndef ATLAS_BUFFER_DISABLE_THREADSAFE

#include <pthread.h>

#endif
namespace ATLAS {

/**
 * Declaration for CircularBuffer Class
 * @tparam S Buffer Size
 * @tparam T Buffer Class
 */

    template<size_t S, class T>
    class CircularBuffer {
    private:
        T _vec[S];                  // Vector containing buffer values
        size_t _bufferIndex;        // Current index of buffer
#ifndef ATLAS_BUFFER_DISABLE_THREADSAFE
// Supress C++-98 compatibility warning
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wc++98-compat"
#pragma GCC diagnostic ignored "-Wc++11-extensions"
        pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;
#pragma GCC pop
#pragma GCC pop
#endif
    public:

        /**
         * CircularBuffer Constructor
         * Creates a null circular buffer
         */

        CircularBuffer() {
            this->_bufferIndex = -1;
        }

        /**
         * Sze of buffer
         * @return size
         */

        size_t size() {
            return S;
        }

        /**
         * Insert element into buffer
         * @param value Value to be added into buffer
         */

        void insert(T value) {
            if (pthread_mutex_lock(&this->_mutex) == 0) {
                this->_vec[++this->_bufferIndex %= S] = value;
            }
            pthread_mutex_unlock(&this->_mutex);
        }

        /**
         * Read most recent element
         */

        T at() {
            T retVal = 0;
            if (pthread_mutex_lock(&this->_mutex) == 0) {
                retVal = this->_vec[this->_bufferIndex];
            }
            pthread_mutex_unlock(&this->_mutex);
            //assert(retVal != 0);
            return retVal;
        }

        /**
         * Read element in buffer
         * @param index Index of value
         */

        T at(size_t index) {
            return this->_vec[index %= S];
        }

        /**
         * Fill bufffer with zeros
         */

        void scrub() {
            this->_bufferIndex = 0;
            size_t i;
            for (i = 0; i < S; ++i) {
                this->insert(0);
            }
        }

        /**
         * Returns Buffer MUTEX
         */

        pthread_mutex_t *mutex() {
            return &this->_mutex;
        }

    };
}

#endif /* CircularBuffer_h */