//
// Distributed under the ITensor Library License, Version 1.2
//    (See accompanying LICENSE file.)
//
#ifndef __ITENSOR_READWRITE_H_
#define __ITENSOR_READWRITE_H_

#include <fstream>
#include <memory>
#include <vector>
#include "string.h"
#include "itensor/types.h"
#include "itensor/util/error.h"
#include "itensor/util/infarray.h"

#if defined(_WIN32)
#include <process.h>
#include <io.h>
#include <direct.h>
inline char*
mkdtemp(char *templat)
    {
    char* retval = _mktemp(templat);
    if (retval) { _mkdir(retval); }
    return retval;
    }
#else
#include <unistd.h>
#endif

namespace itensor {

bool inline
fileExists(const std::string& fname)
    {
    std::ifstream file(fname.c_str());
    return file.good();
    }

// Can overload read and write functions with
// signatures below for objects such as std::vector
// 
// For classes having member read/write functions, can 
// leave external read/write overloads undefined
// and the following template overloads will 
// be called
//

////Here we have to use a struct to implement the read(istream,T)
////function because function templates cannot be partially specialized
////template<typename T, bool isPod = std::is_pod<T>::value>
//template<typename T, bool isPod = std::is_pod<T>::value>
//struct DoRead
//    {
//    DoRead(std::istream& s, T& obj)
//        {
//        obj.read(s);
//        }
//    };
//template<typename T>
//struct DoRead<T, true>
//    {
//    DoRead(std::istream& s, T& val)
//        {
//        s.read((char*) &val, sizeof(val));
//        }
//    };

template<typename T>
auto
read(std::istream& s, T & val)
    -> stdx::if_compiles_return<void,decltype(val.read(s))>
    {
    val.read(s);
    }

template<typename T>
auto
read(std::istream& s, T & val)
    -> stdx::enable_if_t<std::is_pod<T>::value,void>
    {
    s.read((char*) &val, sizeof(val));
    }

template<typename T, typename... CtrArgs>
T
read(std::istream& s, CtrArgs&&... args)
    {
    T t(std::forward<CtrArgs>(args)...);
    //DoRead<T>(s,t);
    read(s,t);
    return t;
    }

//template<typename T, bool isPod = std::is_pod<T>::value>
//struct DoWrite
//    {
//    DoWrite(std::ostream& s, const T& obj)
//        {
//        obj.write(s);
//        }
//    };
//template<typename T>
//struct DoWrite<T, true>
//    {
//    DoWrite(std::ostream& s, const T& val)
//        {
//        s.write((char*) &val, sizeof(val));
//        }
//    };

namespace detail {

    template<typename T>
    auto
    writeImpl(stdx::choice<1>, std::ostream& s, T const& t)
        -> stdx::enable_if_t<std::is_pod<T>::value,void>
        {
        s.write((char*) &t, sizeof(t));
        }
    template<typename T>
    auto
    writeImpl(stdx::choice<2>, std::ostream& s, T const& t)
        -> stdx::if_compiles_return<void,decltype(t.write(s))>
        {
        t.write(s);
        }
    template<typename T>
    void
    writeImpl(stdx::choice<3>, std::ostream& s, T const& t)
        {
        Error("Object does not define .write method");
        }
}

template<typename T>
void
write(std::ostream& s, T const& val)
    {
    detail::writeImpl(stdx::select_overload{},s,val);
    }


void inline
write(std::ostream& s, const std::string& str)
    {
    auto size = str.size();
    s.write((char*)&size,sizeof(size));
    s.write((char*)str.data(),sizeof(char)*size);
    }

void inline
read(std::istream& s, std::string& str)
    {
    auto size = str.size(); //will overwrite
    s.read((char*)&size,sizeof(size));
    str.resize(size);
    s.read((char*)str.data(),sizeof(char)*size);
    }

void inline
read(std::istream& s, Cplx& z)
    {
    auto &r = reinterpret_cast<Real(&)[2]>(z)[0];
    auto &i = reinterpret_cast<Real(&)[2]>(z)[1];
    s.read((char*)&r,sizeof(r));
    s.read((char*)&i,sizeof(i));
    }

void inline
write(std::ostream& s, const Cplx& z)
    {
    auto &r = reinterpret_cast<const Real(&)[2]>(z)[0];
    auto &i = reinterpret_cast<const Real(&)[2]>(z)[1];
    s.write((char*)&r,sizeof(r));
    s.write((char*)&i,sizeof(i));
    }

template<typename T>
auto
read(std::istream& s, std::vector<T> & v)
    -> stdx::if_compiles_return<void,decltype(itensor::read(s,v[0]))>
    {
    auto size = v.size();
    itensor::read(s,size);
    v.resize(size);
    if(std::is_pod<T>::value)
        {
        s.read((char*)v.data(), sizeof(T)*size);
        }
    else
        {
        for(auto& el : v) itensor::read(s,el);
        }
    }


template<typename T>
auto
write(std::ostream& s, std::vector<T> const& v)
    -> stdx::if_compiles_return<void,decltype(itensor::write(s,v[0]))>
    {
    auto size = v.size();
    itensor::write(s,size);
    if(std::is_pod<T>::value)
        {
        s.write((char*)v.data(), sizeof(T)*size);
        }
    else
        {
        for(auto& el : v) itensor::write(s,el);
        }
    }


//////////////////////////////////////////////
//////////////////////////////////////////////
//////////////////////////////////////////////

template<typename T, size_t N, bool isPod = std::is_pod<T>::value>
struct ReadIAData
    {
    ReadIAData(size_t size, std::istream& s, InfArray<T,N>& ia)
        {
        for(auto& el : ia) itensor::read(s,el);
        }
    };
template<typename T, size_t N>
struct ReadIAData<T,N,/*isPod==*/true>
    {
    ReadIAData(size_t size, std::istream& s, InfArray<T,N>& ia)
        {
        s.read((char*)ia.data(), sizeof(T)*size);
        }
    };
template<typename T, size_t N>
void
read(std::istream& s, InfArray<T,N>& ia)
    {
    decltype(ia.size()) size = 0;
    itensor::read(s,size);
    ia.resize(size);
    ReadIAData<T,N>(size,s,ia);
    }

template<typename T, size_t N, bool isPod = std::is_pod<T>::value>
struct WriteIAData
    {
    WriteIAData(size_t size, std::ostream& s, const InfArray<T,N>& ia)
        {
        for(auto& el : ia) itensor::write(s,el);
        }
    };
template<typename T, size_t N>
struct WriteIAData<T,N,/*isPod==*/true>
    {
    WriteIAData(size_t size, std::ostream& s, const InfArray<T,N>& ia)
        {
        s.write((char*)ia.data(), sizeof(T)*size);
        }
    };
template<typename T, size_t N>
void
write(std::ostream& s, const InfArray<T,N>& ia)
    {
    auto size = ia.size();
    itensor::write(s,size);
    WriteIAData<T,N>(size,s,ia);
    }


//////////////////////////////////////////////
//////////////////////////////////////////////

template<class T> 
void
readFromFile(const std::string& fname, T& t) 
    { 
    std::ifstream s(fname.c_str(),std::ios::binary);
    if(!s.good()) 
        throw ITError("Couldn't open file \"" + fname + "\" for reading");
    read(s,t); 
    s.close(); 
    }


template<class T, typename... InitArgs>
T
readFromFile(const std::string& fname, InitArgs&&... iargs)
    { 
    std::ifstream s(fname.c_str(),std::ios::binary); 
    if(!s.good()) 
        throw ITError("Couldn't open file \"" + fname + "\" for reading");
    T t(std::forward<InitArgs>(iargs)...);
    read(s,t); 
    s.close(); 
    return t;
    }


template<class T> 
void
writeToFile(const std::string& fname, const T& t) 
    { 
    std::ofstream s(fname.c_str(),std::ios::binary); 
    if(!s.good()) 
        throw ITError("Couldn't open file \"" + fname + "\" for writing");
    write(s,t); 
    s.close(); 
    }

//Given a prefix (e.g. pfix == "mydir")
//and an optional location (e.g. locn == "/var/tmp/")
//creates a temporary directory and returns its name
//without a trailing slash
//(e.g. /var/tmp/mydir_SfqPyR)
std::string inline
mkTempDir(const std::string& pfix,
          const std::string& locn = "./")
    {
    //Construct dirname
    std::string dirname = locn;
    if(dirname[dirname.length()-1] != '/')
        dirname += '/';
    //Add prefix and template string of X's for mkdtemp
    dirname += pfix + "_XXXXXX";

    //Create C string version of dirname
    auto cstr = std::unique_ptr<char[]>(new char[dirname.size()+1]);
    strcpy(cstr.get(),dirname.c_str());

    //Call mkdtemp
    char* retval = mkdtemp(cstr.get());
    //Check error condition
    if(retval == NULL) throw ITError("mkTempDir failed");

    //Prepare return value
    std::string final_dirname(retval);

    return final_dirname;
    }

} // namespace itensor

#endif
