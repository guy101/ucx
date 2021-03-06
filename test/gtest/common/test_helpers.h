/**
* Copyright (C) Mellanox Technologies Ltd. 2001-2012.  ALL RIGHTS RESERVED.
* Copyright (c) UT-Battelle, LLC. 2015. ALL RIGHTS RESERVED.
*
* See file LICENSE for terms.
*/

#ifndef UCS_TEST_HELPERS_H
#define UCS_TEST_HELPERS_H

#include <ucs/sys/preprocessor.h>
#include "gtest.h"
#include <errno.h>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <string>

namespace ucs {

class test_abort_exception : public std::exception {
};


class exit_exception : public std::exception {
public:
    exit_exception(bool failed) : m_failed(failed) {
    }

    virtual ~exit_exception() throw() {
    }

    bool failed() const {
        return m_failed;
    }

private:
    const bool m_failed;
}
;

class test_skip_exception : public std::exception {
public:
    test_skip_exception(const std::string& reason = "") : m_reason(reason) {
    }
    virtual ~test_skip_exception() throw() {
    }

    virtual const char* what() const throw() {
        return m_reason.c_str();
    }

private:
    const std::string m_reason;
};


/**
 * @return Time multiplier for performance tests.
 */
int test_time_multiplier();


/**
 * Signal-safe sleep.
 */
void safe_usleep(double usec);


/*
 * For gtest's EXPECT_EQ
 */
template <typename T>
static std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec) {
    static const size_t LIMIT = 2000;
    size_t i = 0;
    for (std::vector<char>::const_iterator iter = vec.begin(); iter != vec.end(); ++iter) {
        if (i >= LIMIT) {
            os << "...";
            break;
        }
        os << "[" << i << "]=" << *iter << " ";
        ++i;
    }
    return os << std::endl;
}

std::ostream& operator<<(std::ostream& os, const std::vector<char>& vec);

template <typename OutputIterator>
static void fill_random(OutputIterator begin, OutputIterator end) {
    for (OutputIterator iter = begin; iter != end; ++iter) {
        *iter = rand();
    }
}

template <typename T>
static inline T random_upper() {
  return static_cast<T>((rand() / static_cast<double>(RAND_MAX)) * std::numeric_limits<T>::max());
}

template <typename T>
class hex_num {
public:
    hex_num(const T num) : m_num(num) {
    }

    operator T() const {
        return m_num;
    }

    template<typename N>
    friend std::ostream& operator<<(std::ostream& os, const hex_num<N>& h);
private:
    const T m_num;
};

template <typename T>
hex_num<T> make_hex(const T num) {
    return hex_num<T>(num);
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const hex_num<T>& h) {
    return os << std::hex << h.m_num << std::dec;
}
class scoped_setenv {
public:
    scoped_setenv(const char *name, const char *value);
    ~scoped_setenv();
private:
    scoped_setenv(const scoped_setenv&);
    const std::string m_name;
    std::string       m_old_value;
};

template <typename T>
std::string to_string(const T& value) {
    std::stringstream ss;
    ss << value;
    return ss.str();
}

template <typename T>
class ptr_vector_base {
public:
    typedef std::vector<T*> vec_type;
    typedef typename vec_type::const_iterator const_iterator;

    ptr_vector_base() {
    }

    virtual ~ptr_vector_base() {
        clear();
    }

    /** Add and take ownership */
    void push_back(T* ptr) {
        m_vec.push_back(ptr);
    }

    void push_front(T* ptr) {
        m_vec.insert(m_vec.begin(), ptr);
    }

    virtual void clear() {
        while (!m_vec.empty()) {
            T* ptr = m_vec.back();
            m_vec.pop_back();
            release(ptr);
        }
    }

    const_iterator begin() const {
        return m_vec.begin();
    }

    const_iterator end() const {
        return m_vec.end();
    }

    T* front() {
        return m_vec.front();
    }

    T* back() {
        return m_vec.back();
    }

    size_t size() const {
        return m_vec.size();
    }

protected:
    ptr_vector_base(const ptr_vector_base&);
    vec_type m_vec;

private:
    void release(T *ptr) {
        delete ptr;
    }
};

template<> inline void ptr_vector_base<void>::release(void *ptr) {
    free(ptr);
}


template <typename T>
class ptr_vector : public ptr_vector_base<T> {
public:
    T& at(size_t index) const {
        return *ptr_vector_base<T>::m_vec.at(index);
    }
};

template <>
class ptr_vector<void> : public ptr_vector_base<void> {
};


/**
 * Safely wraps C handles
 */
template <typename T>
class handle {
public:
    typedef T handle_type;
    typedef void (*dtor_t)(T handle);

    handle() : m_initialized(false), m_value(NULL), m_dtor(NULL) {
    }

    handle(const T& value, dtor_t dtor) : m_initialized(true), m_value(value), m_dtor(dtor) {
        ucs_assert(value != NULL);
    }

    handle(const handle& other) : m_initialized(false), m_value(NULL), m_dtor(NULL) {
        *this = other;
    }

    ~handle() {
        reset();
    }

    void reset() {
        if (m_initialized) {
            release();
        }
    }

    void revoke() const {
        m_initialized = false;
    }

    void reset(const T& value, dtor_t dtor) {
        reset();
        if (value == NULL) {
            throw std::invalid_argument("value cannot be NULL");
        }
        m_value = value;
        m_dtor  = dtor;
        m_initialized = true;
    }

    const handle& operator=(const handle& other) {
        reset();
        if (other.m_initialized) {
            reset(other.m_value, other.m_dtor);
            other.revoke();
        }
        return *this;
    }

    operator T() const {
        return m_value;
    }

    operator bool() const {
        return m_initialized;
    }

    T get() const {
        return m_value;
    }

private:

    void release() {
        m_dtor(m_value);
        m_initialized = false;
    }

    mutable bool   m_initialized;
    T              m_value;
    dtor_t         m_dtor;
};

#define UCS_TEST_CREATE_HANDLE(_t, _handle, _dtor, _ctor, ...) \
    { \
        _t h; \
        ucs_status_t status = _ctor(__VA_ARGS__, &h); \
        ASSERT_UCS_OK(status); \
        _handle.reset(h, _dtor); \
    }


class size_value {
public:
    explicit size_value(size_t value) : m_value(value) {}

    size_t value() const {
        return m_value;
    }
private:
    size_t m_value;
};


template <typename O>
static inline O& operator<<(O& os, const size_value& sz)
{
    size_t v = sz.value();

    std::iostream::fmtflags f(os.flags());

    /* coverity[format_changed] */
    os << std::fixed << std::setprecision(1);
    if (v < 1024) {
        os << v;
    } else if (v < 1024 * 1024) {
        os << (v / 1024.0) << "k";
    } else if (v < 1024 * 1024 * 1024) {
        os << (v / 1024.0 / 1024.0) << "m";
    } else {
        os << (v / 1024.0 / 1024.0 / 1024.0) << "g";
    }

    os.flags(f);
    return os;
}


namespace detail {

class message_stream {
public:
    message_stream(const std::string& title);
    ~message_stream();

    template <typename T>
    std::ostream& operator<<(const T& value) const {
        return std::cout << value;
    }

    std::iostream::fmtflags flags() {
        return std::cout.flags();
    }

    void flags(std::iostream::fmtflags f) {
        std::cout.flags(f);
    }
};

} // detail

} // ucs


/* Test output */
#define UCS_TEST_MESSAGE \
    ucs::detail::message_stream("INFO")


/* Skip test */
#define UCS_TEST_SKIP \
    do { \
        throw ucs::test_skip_exception(); \
    } while(0)
#define UCS_TEST_SKIP_R(_reason) \
    do { \
        throw ucs::test_skip_exception(_reason); \
    } while(0)


/* Abort test */
#define UCS_TEST_ABORT(_message) \
    do { \
        std::stringstream ss; \
        ss << _message; \
        GTEST_MESSAGE_(ss.str().c_str(), ::testing::TestPartResult::kFatalFailure); \
        throw ucs::test_abort_exception(); \
    } while(0)


/* UCS error check */
#define EXPECT_UCS_OK(_error)  EXPECT_EQ(UCS_OK, _error) << "Error: " << ucs_status_string(_error)
#define ASSERT_UCS_OK(_error, ...) \
    do { \
        if ((_error) != UCS_OK) { \
            UCS_TEST_ABORT("Error: " << ucs_status_string(_error)  __VA_ARGS__); \
        } \
    } while (0)

#define ASSERT_UCS_OK_OR_INPROGRESS(_error) \
    do { \
        if ((_error) != UCS_OK && (_error) != UCS_INPROGRESS) { \
            UCS_TEST_ABORT("Error: " << ucs_status_string(_error)); \
        } \
    } while (0)


/* Run code block with given time limit */
#define UCS_TEST_TIME_LIMIT(_seconds) \
    for (ucs_time_t _start_time = ucs_get_time(), _elapsed = 0; \
         _start_time != 0; \
         (ucs_time_to_sec(_elapsed = ucs_get_time() - _start_time) >= \
                         (_seconds) * ucs::test_time_multiplier()) \
                         ? (GTEST_NONFATAL_FAILURE_("Time limit exceeded:") << \
                                         "Expected time: " << ((_seconds) * ucs::test_time_multiplier()) << " seconds\n" << \
                                         "Actual time: " << ucs_time_to_sec(_elapsed) << " seconds", 0) \
                         : 0, \
              _start_time = 0)


/**
 * Scoped exit for C++. Usage:
 *
 * UCS_TEST_SCOPE_EXIT() { <code> } UCS_TEST_SCOPE_EXIT_END
 */
#define _UCS_TEST_SCOPE_EXIT(_classname, ...) \
    class _classname { \
    public: \
        _classname() {} \
        ~_classname()
#define UCS_TEST_SCOPE_EXIT(...) \
    _UCS_TEST_SCOPE_EXIT(UCS_PP_APPEND_UNIQUE_ID(onexit), ## __VA_ARGS__)

#define UCS_TEST_SCOPE_EXIT_END \
    } UCS_PP_APPEND_UNIQUE_ID(onexit_var);


#endif
