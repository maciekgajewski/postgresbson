// Copyright (c) 2012-2013 Maciej Gajewski <maciej.gajewski0@gmail.com>
// 
// Permission to use, copy, modify, and distribute this software and its documentation for any purpose, without fee, and without a written agreement is hereby granted,
// provided that the above copyright notice and this paragraph and the following two paragraphs appear in all copies.
// 
// IN NO EVENT SHALL THE AUTHOR BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, 
// ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE AUTHOR HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// THE AUTHOR SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
// THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND THE AUTHOR HAS NO OBLIGATIONS TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

#ifndef PGBSON_INTERNAL_HPP
#define PGBSON_INTERNAL_HPP

#include "mongo/db/jsobj.h"
#include "mongo/db/json.h"
#ifdef LOG
#undef LOG
#endif

extern "C" {
#include "postgres.h"
#include "fmgr.h"
#include "executor/executor.h"
#include "utils/typcache.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"
#include "utils/timestamp.h"
#include "parser/parse_coerce.h"
#include "catalog/pg_type.h"
#include "funcapi.h"

// bson access macros
#define DatumGetBson(X) ((bytea *) PG_DETOAST_DATUM_PACKED(X))
#define GETARG_BSON(n)  DatumGetBson(PG_GETARG_DATUM(n))

}

#include <string>

// logging (to stdout)
#ifdef PGBSON_LOGGING
    #define PGBSON_LOG std::cout
    #define PGBSON_ENDL std::endl
#else
    // dummy object with << operator for all types
    struct null_stream {};

    template<typename T>
    const null_stream& operator << (const null_stream& l, const T&) { return l; }

    #define PGBSON_LOG null_stream()
    #define PGBSON_ENDL 0

#endif

// postgres helpers
Datum return_string(const std::string& s);
Datum return_cstring(const std::string& s);
Datum return_bson(const mongo::BSONObj& b);

inline mongo::BSONObj datum_get_bson(Datum* val)
{
    bytea* data = DatumGetBson(val);
    return mongo::BSONObj(VARDATA_ANY(data));
}

std::string get_typename(Oid typid);

// bson object inspection


template<typename FieldType> // field type is not really used anywher, it's just used for selecting proper convert function etc
Datum convert_element(PG_FUNCTION_ARGS, const mongo::BSONElement e); // never implemented

template<>
Datum convert_element<std::string>(PG_FUNCTION_ARGS, const mongo::BSONElement e);

template<>
Datum convert_element<int>(PG_FUNCTION_ARGS, const mongo::BSONElement e);

template<>
Datum convert_element<double>(PG_FUNCTION_ARGS, const mongo::BSONElement e);

template<>
Datum convert_element<int64>(PG_FUNCTION_ARGS, const mongo::BSONElement e);

// exception usedit indicate conversion error
struct convertion_error
{
    const char* target_type;
    convertion_error(const char* tt) : target_type(tt) { }
};

const char* bson_type_name(const mongo::BSONElement& e);

template<typename FieldType>
static
Datum bson_get(PG_FUNCTION_ARGS)
{
    bytea* arg = GETARG_BSON(0);
    mongo::BSONObj object(VARDATA_ANY(arg));

    text* arg2 = PG_GETARG_TEXT_P(1);
    std::string field_name(VARDATA(arg2),  VARSIZE(arg2)-VARHDRSZ);

    PGBSON_LOG << "bson_get: field: " << field_name << PGBSON_ENDL;
    mongo::BSONElement e = object.getFieldDotted(field_name);
    if (e.eoo())
    {
        PGBSON_LOG << "bson_get: no such field" << PGBSON_ENDL;
        // no such element
        PG_RETURN_NULL();
    }
    else
    {
        try
        {
            return convert_element<FieldType>(fcinfo, e);
        }
        catch(const convertion_error& ex)
        {
            ereport(
                ERROR,
                    (
                    errcode(ERRCODE_INTERNAL_ERROR),
                    errmsg("Field %s is of type %s and can not be converted to %s",
                        field_name.c_str(), bson_type_name(e), ex.target_type)
                    )
                );
        }
        catch(const std::exception& ex)
        {
            ereport(
                ERROR,
                    (
                    errcode(ERRCODE_INTERNAL_ERROR),
                    errmsg("Error converting filed %s of type %s: %s",
                        field_name.c_str(), bson_type_name(e), ex.what())
                    )
                );
        }
    }
}

// bson manipulation/creation

void composite_to_bson(mongo::BSONObjBuilder& builder, Datum composite);

void datum_to_bson(const char* field_name, mongo::BSONObjBuilder& builder,
    Datum val, bool is_null, Oid typid);

#endif
