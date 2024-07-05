#include "Namespaces.h"
#include "Types.h"
#include "MathUtil.h"
#include "Hash.h"
#include "Dict.h"
#include "Char.h"


_MUON_NAMESPACE_BEGIN


/*! @brief Fetch value by the key or throw exception if key not found
 * @return Item associated with the key
 * @throws KeyErrorExeption in case no key found
 */
Variant&
BaseDictManager::operator [ ](const char *akey)
{
    DictItem *item;
    Variant *tmp;

    tmp = lookUp(akey);
    if(!tmp) {
        item = addItem(akey, Variant());
        tmp = &item->value;
    }

    return *tmp;
}


/*! @brief Fetch value by the key or throw exception if key not found (const version)
 * @return Item associated with the key
 * @throws KeyErrorException in case no key found
 */
const Variant&
BaseDictManager::operator [ ](const char *akey) const
{
    const Variant *tmp;

    tmp = lookUp(akey);
    if(!tmp)
        throw KeyErrorException(akey);

    return *tmp;
}


const Variant&
BaseDictManager::get(const char *akey, const Variant& _default) const
{
    const Variant *tmp;
    tmp = lookUp(akey);
    return tmp? *tmp: _default;
}


void
BaseDictManager::copy(const BaseDictManager& other, const char *key)
{
    const Variant *tmp;

    tmp = other.lookUp(key);
    if(tmp)
        addItem(key, *tmp);
}

/*! @brief Remove item with a given key from dictionary
 * @param akey Key of the item to remove
 * @return true if item removed, false otherwise (not found, etc.)
 */
bool
BaseDictManager::removeItem(const char *akey) 
{
    REF_PARAM(akey);
    throw NotImplementedException();
}


/*! @brief Add item with given key to dicitionary
 * @param akey Key of the item
 * @param val Item value
 * @return Pointer to added item or NULL on error
 */
DictItem *
BaseDictManager::addItem(const char *akey, const Variant& val)
{
    REF_PARAM(akey);
    REF_PARAM(val);
    throw NotImplementedException();
}


/*! @brief Looks up item in dictionary by given key
 * @param akey Key of item to look up
 * @return Pointer to item vaule or NULL if item not found
 */
Variant *
BaseDictManager::lookUp(const char *akey)
{
    REF_PARAM(akey);
    throw NotImplementedException();
}


const Variant *
BaseDictManager::lookUp(const char *akey) const
{
    REF_PARAM(akey);
    throw NotImplementedException();
}


/*! @brief Placeholder to enumerate through dictionary key/value pairs
 *  @param callback User-supplied callback function
 *  @param cbkData  Custom data passed to user-supplied callback function
 *  @return Number of iterations made or -1 on error
 */
int
BaseDictManager::EnumItems(DictEnumCallback callback, void *cbkData)
{
    REF_PARAM(callback);
    REF_PARAM(cbkData);
    throw NotImplementedException();
}


/*! @brief Linear dictionary manager constructor
 */
LinearDictManager::LinearDictManager()
{
    lsdhAlloc = 8;
    lsdhSize = 0;
    lsdhHead = new DictItem[lsdhAlloc];
}


/*! @brief Linear dictionary manager destructor
 */
LinearDictManager::~LinearDictManager()
{
    delete[] lsdhHead;
}


size_t
LinearDictManager::allocItems(size_t nItems)
{
    DictItem *temp;
    size_t nsize, i;

    if(lsdhSize+nItems >= lsdhAlloc) {
        nsize = _round(lsdhSize + nItems, 8);

        // FIXME Make a copying of existing items more wise
        temp = new DictItem[nsize];
        for(i=0; i<lsdhSize; i++)
            temp[i] = lsdhHead[i];

        delete[] lsdhHead;
        lsdhHead = temp;
        lsdhAlloc = nsize;
    }

    return lsdhSize++;
}

/*! @brief Remove item from dictionary
 *  @param akey Key of the item to remove
 *  @returns true on success, false if item with given key not found in dictionary
 */
bool
LinearDictManager::removeItem(const char *akey)
{
    REF_PARAM(akey);
    throw NotImplementedException();
}


/*! @brief Add a key/value pair to the dictionary
 *  @param akey Key string
 *  @param val Value to add
 *  @return true if item sucessfully added, or false otherwise (no memory, etc.)
 */
DictItem *
LinearDictManager::addItem(const char *akey, const Variant& val)
{
    size_t npos;

    try {
        npos = allocItems(1);
    }

    catch(Exception e) {
        return false;
    }

    lsdhHead[npos] = DictItem(akey, val);
    return &lsdhHead[npos];
}


/*! @brief Look up item in dictionary by given key
 *  @param akey Key of the item to look up
 *  @returns Pointer to item, or NULL if item not found in dictionary
 */
Variant *
LinearDictManager::lookUp(const char *akey)
{
    size_t i;

    for(i=0; i<lsdhSize; i++)
        if(!strcmp(lsdhHead[i].getKey(), akey))
            return &lsdhHead[i].value;

    return NULL;
}


const Variant *
LinearDictManager::lookUp(const char *akey) const
{
    size_t i;

    for(i=0; i<lsdhSize; i++)
        if(!strcmp(lsdhHead[i].getKey(), akey))
            return &lsdhHead[i].value;

    return NULL;
}

/**
 * Enumerates through dictionary key/value pairs, passing it to callback function
 * @param callback User-supplied callback function
 * @param data Custom data passed to user callback function
 * @return Number of items enumerated or -1 on error
 */
int
LinearDictManager::EnumItems(DictEnumCallback callback, void *data)
{
    size_t i;

    for(i=0; i<lsdhSize; i++)
        if(!callback(lsdhHead[i], data))
            break;

    return (int)i;
}


HashDictBucket::HashDictBucket(): hdbNext(NULL), hdbCount(0)
{
    /* Dummy */
}


HashDictBucket::~HashDictBucket()
{
    HashDictBucket *bucket, *last;

   /* Free chained buckets */
    for(last=NULL, bucket=hdbNext; bucket; bucket=last) {
        last = bucket->hdbNext;
        delete bucket;
    }
}


DictItem&
HashDictBucket::operator [ ](int i)
{
    if(i<0 || i>NUM_DICT_BUCKET_ITEMS)
        throw IndexErrorException();
    return hdbItems[i];
}


const DictItem&
HashDictBucket::operator [ ](int i) const
{
    if(i<0 || i>NUM_DICT_BUCKET_ITEMS)
        throw IndexErrorException();
    return hdbItems[i];
}


DictItem *
HashDictBucket::lookUp(const char *akey)
{
    int i;
    HashDictBucket *bucket;

    for(bucket=this; bucket; bucket=bucket->hdbNext) {
        for(i=0; i<bucket->hdbCount; i++) {
            if(!strcmp(akey, bucket->hdbItems[i].getKey()))
                return &bucket->hdbItems[i];
        }
    }

    return NULL;
}


DictItem *
HashDictBucket::addItem(const DictItem& src)
{
    HashDictBucket *ptr, *last;
    DictItem *item;

    /* Walk the bucket chain in order to find free place for item
     */
    for(last=NULL, ptr=this; ptr; last=ptr, ptr=ptr->hdbNext)
        if(ptr->hdbCount < NUM_DICT_BUCKET_ITEMS)
            break;

    /* If no room found, allocate a new bucket and add it to the
     * bucket chain
     */
    if(!ptr)
        ptr = (last->hdbNext = new HashDictBucket());

    /* Copy item into bucket */
    item = &ptr->hdbItems[ptr->hdbCount++];
    *item = src;

    return item;
}


int
HashDictBucket::EnumItems(DictEnumCallback callback, void *data)
{
    HashDictBucket *bucket;
    int i, total;

    for(total=0, bucket=this; bucket; bucket=bucket->hdbNext)
        for(i=0; i<bucket->hdbCount; i++, total++)
            if(!callback(bucket->hdbItems[i], data))
                return total;

    return total;
}


HashDictManager::HashDictManager()
{
    int i;
    for(i=0; i<DICT_HASH_SIZE; i++)
        hsdhHash[i] = NULL;
}


HashDictManager::~ HashDictManager()
{
    int i;

    for(i=0; i<DICT_HASH_SIZE; i++)
        delete hsdhHash[i];
}


DictItem *
HashDictManager::addItem(const char *akey, const Variant& val)
{
    unsigned int hk;

    hk = hash_calcstr(akey, DICT_HASH_SIZE);
    if(hsdhHash[hk] == NULL)
        hsdhHash[hk] = new HashDictBucket();

    return hsdhHash[hk]->addItem(DictItem(akey, val));
}


bool
HashDictManager::removeItem(const char *akey)
{
    REF_PARAM(akey);
    throw NotImplementedException();
}


Variant *
HashDictManager::lookUp(const char *akey)
{
    unsigned int hk;
    DictItem *item;

    hk = hash_calcstr(akey, DICT_HASH_SIZE);
    if(hsdhHash[hk] == NULL)
        return NULL;

    item = hsdhHash[hk]->lookUp(akey);
    if(item == NULL)
        return NULL;

    return &item->value;
}


const Variant *
HashDictManager::lookUp(const char *akey) const
{
    unsigned int hk;
    DictItem *item;

    hk = hash_calcstr(akey, DICT_HASH_SIZE);
    if(hsdhHash[hk] == NULL)
        return NULL;

    item = hsdhHash[hk]->lookUp(akey);
    if(item == NULL)
        return NULL;

    return &item->value;
}

int
HashDictManager::EnumItems(DictEnumCallback callback, void *data)
{
    int i, count, total;

    for(total=0, i=0; i<DICT_HASH_SIZE; i++) {
        if(hsdhHash[i]) {
            /* Enumerate items in bucket(s) starting in current hash slot
            */
            count = hsdhHash[i]->EnumItems(callback, data);
            if(count < 0) {
                total += -count;
                break;
            }

            total += count;
        }
    }

    return total;
}


char *
Dict::haveMacro(const char *str, size_t *len, char **name, size_t *namelen)
{
    char *pm, *pm2;
    char endch;
    int i;

    pm  = const_cast<char *>(strchr(str, '$'));
    pm2 = const_cast<char *>(strchr(str, '%'));

    if(!pm && !pm2)
        return NULL;

    if(!pm) {
        pm = pm2;
    } else {
        if(pm2 && (pm2 < pm))
            pm = pm2;
    }

    /* $$ represents single '$', %% - '%', so eat it
     */
    if(pm[0]==pm[1]) {
        *len = 2;
        *name = &pm[1];
        *namelen = 1;
        return pm;
    }

    /* After macro character we expect macro name enclosed in
     * curly braces {} or simple braces (), or sequence of
     * alphanumeric characters, possibly starting with underscore '_'
     * and separated by '-', '.' or '_'
     */
    if(pm[1]=='_' || _is_alnum(pm[1])) {
        for(i=1; _is_ident_char(pm[i]); i++) {
            /* Don't allow '--' and '..' sequences
             */
            if(pm[i]=='-' || pm[i]=='.')
                if(pm[i]==pm[i+1])
                    break;
        }

        *len = i;
        *name = &pm[1];
        *namelen = i-1;
        return pm;
    }

    if(pm[1]!='(' && pm[1]!='{')
        return NULL;

    /* Scan string until we reach EOS marker or found macro separator
     */
    endch = (pm[1]=='(')? ')': '}';
    for(i=2; pm[i]!='\0'; i++)
        if(pm[i] == endch)
            break;

    if(pm[i] != endch)
        return NULL;

    *name = &pm[2];
    *namelen = i-2;
    *len = i+1;
    return pm;
}


void
Dict::makeSpace(size_t cch)
{
    char *temp;
    size_t cb;
    size_t blk, rem;

    if(cch == 0)
        return;

    if((xlatPtr+cch) >= xlatAlloc) {
        blk = (xlatPtr + cch) / DICT_XLAT_PAGESIZE;
        rem = (xlatPtr + cch) % DICT_XLAT_PAGESIZE;
        cb = (blk * DICT_XLAT_PAGESIZE) + (rem? DICT_XLAT_PAGESIZE: 0);

        temp = (char *)realloc(xlatBuf, cb*sizeof(xlatBuf[0]));
        if(!temp)
            /* probably will never happen on modern OS-es */
            throw MemoryErrorException();

        xlatBuf = temp;
        xlatAlloc = cb;
    }
}


/**
 * Add string to the buffer
 * @param str Pointer to string
 * @param len number of characters in the string
 * @throws MemoryErrorException In case of not enough memory
 */
void
Dict::addChars(const char *str, size_t len)
{
    if(len > 0) {
        makeSpace(len); // throws an exception in case of memory error
#ifdef _MSC_VER
#if _MSC_VER > 1310
        strncpy_s(&xlatBuf[xlatPtr], xlatAlloc-xlatPtr, str, len);
#else
        strncpy(&xlatBuf[xlatPtr], str, len);
#endif
#else
        strncpy(&xlatBuf[xlatPtr], str, len);
#endif
        xlatPtr += len;
    }
}


void
Dict::expand(const char *str, size_t len)
{
    char temp[MAX_MACRO_LEN+1];
    char *pm, *name, *stritem;
    size_t mlen, cch, namelen;
    Variant *item;

    /* find macro name in given string
     */
    while((len != 0) && (pm = haveMacro(str, &mlen, &name, &namelen))!=NULL) {
        /* Found macro. Split the string into two parts: a string before
         * macro and macro definition
         */
        addChars(str, pm-str);

        /* Avoid buffer overflows: use maximum MAX_MACRO_LEN 
         * characters of macro name
         */
        if(namelen > MAX_MACRO_LEN)
            namelen = MAX_MACRO_LEN;

        /* Check for special case: %% or $$ macros represents % and $ respectively
         */
        if(namelen==1 && (*name=='%' || *name=='$')) {
            addChars(name, namelen);
        } else {
            /* Look up and fetch macro value
             */
#if _MSC_VER >= 1400
            strncpy_s(temp, sizeof(temp)-1, name, namelen);
            temp[namelen] = '\0';
#else
            strncpy(temp, name, namelen);
            temp[namelen] = '\0';
#endif

            item = lookUp(temp);
            if(item) {
                stritem = (char *)(*item);
                if(stritem)
                    expand(stritem, strlen(stritem));
            } else {
                addChars(pm, mlen);
            }
        }

        cch = &pm[mlen] - str;
        str = &pm[mlen];
        len -= cch;
    }

    /* Add remaining characters */
    addChars(str, len);
}


/*! @brief Translates given string with regards to nested macro definitions
 *  @param buf Destination buffer or NULL if buffer need to be dynamically allocated
 *  @param cch Number of characters in destination buffer or 0
 *  @param str String to be translated
 *  @return Translated string or NULL on error
 * Caller have multiple choices:
 * 1) Supply buffer address and size - resulting string will be truncated if his length
 *    exceeds specified limits. If size are 0 then function will silently fail.
 * 2) Support NULL as buffer address and non-zero 'cch' size - fixed-size buffer will be
 *    allocated and string will be truncated if length exceeds limits.
 * 3) Supply NULL as buffer address and 0 as buffer size - buffer will be allocated and
 *    dynamically grow to accomodate resulting string length.
 */
char *
Dict::xlat(const char *str, u_int flags)
{
    Variant *item;
    char *itemstr;

    REF_PARAM(flags); // for future use. see XLAT_PATH_SEMANTICS

    resetXlat();

    item = lookUp(str);
    if(!item) {
        /* If no such key found, try to expand possible macros inside the string */
        expand(str, strlen(str));
    } else {
        /* Convert item to string and perform macro expansion */
        itemstr = (char *)(*item);
        expand(itemstr, strlen(itemstr));
    }

    /* Add string termination character */
    makeSpace(1);
    xlatBuf[xlatPtr] = '\0';
    return xlatBuf;
}


_MUON_NAMESPACE_END

