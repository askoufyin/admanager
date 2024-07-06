#ifndef __CORE_DICT_INCLUDED__
#define __CORE_DICT_INCLUDED__


#include <stdlib.h>
#include <stdint.h>


/* Tunable HashDict parameters
 */
#define NUM_DICT_BUCKET_ITEMS   8
#define DICT_HASH_SIZE          256
#define DICT_XLAT_PAGESIZE      1024


/* maximum macro name length, in characters */
#define MAX_MACRO_LEN           128


enum XlatFlags
{
    XLAT_PATH_SEMANTICS = 1     /* Translate / or \ characters into OS-specific variant */
};



/*! \brief Dictionary key/value pair
 */
class DictItem 
{
protected:
    char *key;
public:
    Variant value;
public:
    DictItem(): key(NULL), value() {
    }

    DictItem(const char *akey, const Variant& val) {
        key = __strdup(akey);
        value = val;
    }

    virtual ~DictItem() {
        free(key);
    }

    /*! \brief Get item key */
    virtual const char *getKey() const {
        return key? (const char *)key: "(null)";
    }

    /*! \brief Set item key */
    virtual void setKey(const char *akey) {
        char *newkey = __strdup(akey);
        if(newkey) {
            free(key);
            key = newkey;
        } else
            throw MemoryErrorException();
    }

    virtual DictItem& operator = (const DictItem& other) {
        char *oldkey = this->key;

        key = __strdup(other.key);
        value = other.value;
        free(oldkey);

        return *this;
    }
};


/*! \brief Callback function to enumerate dictionary key/value pairs
 */
typedef bool (*DictEnumCallback)(const DictItem&, void *);


/*! \brief Abstract base class for all dictionary managers
 *
 * Implements functionality to search, add or remove items from dict
 */
class COREAPI BaseDictManager 
{
public:
    BaseDictManager() {}
    virtual ~BaseDictManager() {}
    virtual bool removeItem(const char *);
    virtual DictItem *addItem(const char *, const Variant&);
    virtual Variant *lookUp(const char *);
    virtual const Variant *lookUp(const char *) const;
    virtual const Variant& get(const char *, const Variant& = Variant()) const;
    virtual void copy(const BaseDictManager&, const char *);
    virtual Variant& operator[] (const char *);
    virtual const Variant& operator[] (const char *) const;
    virtual int EnumItems(DictEnumCallback, void * = NULL);
};


/*! \brief Simple linear-search dictionaty manager
 *
 * Useful for managing very small (about 8-16 items) dictionaries.
 * Consumes very little of memory, have O(N) search time
 *
 * \sa BaseDictManager, HashDictManager
 */
class COREAPI LinearDictManager: public virtual BaseDictManager
{
protected:
    DictItem *lsdhHead;
    size_t lsdhSize;
    size_t lsdhAlloc;
    size_t allocItems(size_t);
public:
    LinearDictManager();
    virtual ~LinearDictManager();
    virtual bool removeItem(const char *);
    virtual DictItem *addItem(const char *, const Variant&);
    virtual Variant *lookUp(const char *);
    virtual const Variant *lookUp(const char *) const;
    virtual int EnumItems(DictEnumCallback, void * = NULL);
};


/*! \brief Helper class for hash-based dictionaries
 */
class COREAPI HashDictBucket 
{
protected:
    HashDictBucket *hdbNext;
    int hdbCount;
    DictItem hdbItems[NUM_DICT_BUCKET_ITEMS];
public:
    HashDictBucket();
    virtual ~HashDictBucket();
    virtual DictItem *addItem(const DictItem&);
    virtual DictItem *lookUp(const char *);
    virtual const DictItem& operator[] (int) const;
    virtual DictItem& operator[] (int);
    virtual int EnumItems(DictEnumCallback, void * = NULL);
};


/*! \brief Hash-based dictionary
 *
 * Very useful for storing and accessing medium-to-large (> 100 items) dictionaries.
 *
 * \sa BaseDictManager, LinearDictManager
 */
class COREAPI HashDictManager: public virtual BaseDictManager 
{
protected:
    HashDictBucket *hsdhHash[DICT_HASH_SIZE];
public:
    HashDictManager();
    virtual DictItem *addItem(const char *, const Variant&);
    virtual bool removeItem(const char *);
    virtual Variant *lookUp(const char *);
    virtual const Variant *lookUp(const char *) const;
    virtual int EnumItems(DictEnumCallback, void *);
    virtual ~HashDictManager();
};


/*! \brief New-style dictionary
 *
 * This component implements the second part of dictionary functionality:
 * the reference to abstract 'Dict' into functions and classes
 */
class BaseDict 
{
public:
    BaseDict() {}
    virtual ~BaseDict() {}
    virtual char *xlat(const char *, u_int=0) = 0;
};


/*! \brief This is a common ancestor for all dictionary classes
 *
 * It very useful when your function doesn't care about what subtype of
 * dictionary they take as an argument.
 *
 * \sa BaseDict, BaseDictManager, LinearDictManager, HashDictManager
 */
class COREAPI Dict: public BaseDict, public virtual BaseDictManager 
{
protected:
    // TODO Make this static members, so all dict's share one buffer per application
    char *xlatBuf;
    size_t xlatPtr;
    size_t xlatAlloc;
protected: //members
    void makeSpace(size_t);
    void addChars(const char *, size_t);
    void expand(const char *, size_t);
    void resetXlat() { xlatPtr = 0; }
    char *haveMacro(const char *, size_t *, char **, size_t *);
public:
    Dict(): xlatBuf(0), xlatPtr(0), xlatAlloc(0) {}
    virtual char *xlat(const char *,u_int=0);
    virtual ~Dict() {
        free(xlatBuf);
    }
};

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4250)
#endif


/*! \brief Mix-in class for small dictionaries */
class SmallDict: public Dict, public LinearDictManager {};


/*! \brief Mix-in class form medium and large dictionaries */
class LargeDict: public Dict, public HashDictManager {};


#ifdef _MSC_VER
#pragma warning(pop)
#endif


#endif


