/*
**********************************************************************
*   Copyright (C) 2000-2007, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*/

#include "unicode/utypes.h"
#include "cstring.h"
#include "cmemory.h"
#include "hash.h"
#include "uvector.h"



#ifdef _WIN32

// IMultiLanguage interfaces
//#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
//#include <olestd.h>
#include <mlang.h>
#include <winver.h>

#elif defined(U_DARWIN)

#include <CoreServices/CoreServices.h>

#else

/* else we use iconv */
#include <errno.h>
#include <iconv.h>

#endif

// encoding information from the system

#define CONVERTER_API_IMLANG 1
#define CONVERTER_API_SYSTEM 2

typedef struct encoding_info {
    char web_charset_name[80];
    char charset_description[80];
} encoding_info;


// codepage structure information

#define BYTE_INFO_CONTINUE_AND_END 0x7f
#define BYTE_INFO_CONTINUE 0x7e
#define BYTE_INFO_END 1
#define BYTE_INFO_UNKNOWN 0

#define MAX_BYTE_LEN 8

typedef struct byte_info
{
    int8_t byte[0x100];
} byte_info;

typedef byte_info* byte_info_ptr;

// codepage info generated by this program

typedef struct _cp_info 
{
    int max_byte_size;
    int min_byte_size;
    char default_char[8];
    UChar default_uchar;     // What the default_char maps to
    byte_info_ptr byte_info;
} cp_info;

// platform converter wrapper class

#define CONVERTER_USE_SUBST_CHAR 1  // use built-in substitution char

#ifdef _WIN32

// use supplied substitution char
#define CP_ID_IS_INT 1
#define CONVERTER_USE_DEF_CHAR MLCONVCHARF_USEDEFCHAR
typedef int32_t cp_id;  // codepage identifier

#elif defined(U_DARWIN)

#define CP_ID_IS_INT 1
#define CONVERTER_USE_DEF_CHAR 0
typedef TextEncoding cp_id;  // codepage identifier

#else

#define CP_ID_IS_INT 0
#define CONVERTER_USE_DEF_CHAR 0
typedef char * cp_id;

#endif

class converter {
public:
    converter(cp_id cp, encoding_info *penc_info);
    ~converter();
    
    size_t from_unicode(char* target, char* target_limit, const UChar* source, const UChar* source_limit, unsigned int flags = CONVERTER_USE_DEF_CHAR);
    size_t to_unicode(UChar* target, UChar* target_limit, const char* source, const char* source_limit);
    inline int32_t get_status() { return m_err; }
    int get_cp_info(cp_id cp, cp_info& cp_inf);
    UBool is_lead_byte_probeable() const;
    UBool is_ignorable() const;
    const char *get_name() const {return m_name;}
    const char *get_premade_state_table() const;

    /**
     * Get encodings supported by the system
     * @param encodings UVector of cp_id
     * @param encoding_info UHash of cp_id -> encoding_info*
     */
    static int get_supported_encodings(UVector *encodings, UHashtable *map_encoding_info, int argc = 0, const char* const argv[] = NULL);
    static const char *get_OS_vendor();
    static const char *get_OS_variant();
    static const char *get_OS_interface();
private:
    char *get_default_char(UChar *default_uchar);

    char m_name[100];
    cp_id m_enc_num;
    encoding_info *m_enc_info;
#ifdef WIN32
    IMultiLanguage2 *m_multilanguage;
    HRESULT m_hr;
#elif defined(U_DARWIN)
    TECObjectRef m_tecFromUnicode;
    TECObjectRef m_tecToUnicode;
    cp_info m_cp_inf;
#else
    iconv_t toCP;
    iconv_t toUTF8;
#endif
    int32_t m_err;
};


/* The max Unicode value that we collect information on.
   Windows sometimes supports surrogates depending on the version.
 */
#if defined(U_HPUX) || defined(U_SOLARIS)
/* HP-UX incorrectly truncates surrogates and turns them into fallbacks.
   This includes gb18030. */
/* Solaris incorrectly converts surrogates individually and turns them into
   substitution characters. This includes gb18030 and hkscs. The exception is
   UTF-EBCDIC */
#define MAX_UNICODE_VALUE 0xFFFF
#else
#define MAX_UNICODE_VALUE 0x10FFFF
#endif

/* What is considered the minimum number of mappings before
   it's considered usable.
   FYI INIS-8 seems to be registered with only 72 characters instead of 127.
 */
#define MIN_NUM_MAPPINGS 0x48


