/*
 *
 *  C++ Portable Types Library (PTypes)
 *  Version 1.8.3  Released 25-Aug-2003
 *
 *  Copyright (c) 2001, 2002, 2003 Hovik Melikyan
 *
 *  http://www.melikyan.com/ptypes/
 *  http://ptypes.sourceforge.net/
 *
 */

#include "ptypes.h"


PTYPES_BEGIN


string lowercase(const char* p) 
{
    // we rely on the function locase() which converts one single
    // character to lower case. all locale specific things can be
    // settled down in the future releases.
    string r;
    if (p != nil) 
    {
        setlength(r, strlen(p));
        char* d = unique(r);
        while (*p != 0)
            *d++ = locase(*p++);
    }
    return r;
}


string lowercase(const string& s)
{
    // this function does practically nothing if the string s
    // contains no uppercase characters. once an uppercase character
    // is encountered, the string is copied to another buffer and the 
    // rest is done as usual.

    string r = s;

    // a trick to get a non-const pointer without making
    // the string unique
    char* p = pchar(pconst(r));
    bool u = false;
    int i = 0;
    
    while (*p != 0)
    {
        char c = locase(*p);
        // if the character went lowercase...
        if (c != *p)
        {
            // if the resulting string r is not unique yet...
            if (!u)
            {
                // ... make it unique and adjust the pointer p accordingly
                // this is done only once.
                p = unique(r) + i;
                u = true;
            }
            *p = c;
        }
        p++;
        i++;
    }

    return r;
}


PTYPES_END