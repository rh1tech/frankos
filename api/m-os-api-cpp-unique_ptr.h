#pragma once

template <typename T> class unique_ptr {
    T* p;
    inline unique_ptr(T* p): p(p) {}
public:
/// TODO:
    inline unique_ptr(unique_ptr<T>& up): p(up.p) { up.p = 0; }
    template <typename T2> inline unique_ptr(const unique_ptr<T2>& up) {
        unique_ptr<T2>& up1 = const_cast<unique_ptr<T2>&>(up);
        unique_ptr<T>& up2 = reinterpret_cast<unique_ptr<T>&>(up1);
        this->p = up2.p;
        up2.p = 0;
    }
    inline ~unique_ptr(void) { if (p) delete p; }
    T* operator->() { return p; }
    const T* operator->() const { return p; }
    T& operator*() { return *p; }
    const T& operator*() const { return *p; }
    inline operator bool() const { return !!p; }
    template <typename T2> inline unique_ptr& operator=(const unique_ptr<T2>& up) {
        unique_ptr<T2>& up1 = const_cast<unique_ptr<T2>&>(up);
        unique_ptr<T>& up2 = reinterpret_cast<unique_ptr<T>&>(up1);
        this->p = up2.p;
        up2.p = 0;
        return *this;
    }
    template <typename FT> friend unique_ptr<FT> make_unique();
    template <typename FT, typename A> friend unique_ptr<FT> make_unique(const A& a);
};

template <typename FT> inline unique_ptr<FT> make_unique(void) {
    return unique_ptr<FT>(new FT());
}

template <typename FT, typename A> inline unique_ptr<FT> make_unique(const A& a) {
    return unique_ptr<FT>(new FT(a));
}
