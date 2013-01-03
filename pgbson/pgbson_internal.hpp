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

// bson access macros
#define DatumGetBson(X) ((bytea *) PG_DETOAST_DATUM_PACKED(X))
#define GETARG_BSON(n)  DatumGetBson(PG_GETARG_DATUM(n))

}

#include <string>

// logging (to stdout)
#ifdef PGBSON_LOGGING
    #define PGBSON_LOG std::cout
    #define PGBSON_FLUSH_LOG std::endl
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

std::string get_typename(Oid typid);

// bson object inspection

template<typename FieldType>
static
Datum convert_element(PG_FUNCTION_ARGS, const mongo::BSONElement e);

template<typename FieldType>
static
Datum bson_get(PG_FUNCTION_ARGS)
{
    bytea* arg = GETARG_BSON(0);
    mongo::BSONObj object(VARDATA_ANY(arg));

    text* arg2 = PG_GETARG_TEXT_P(1);

    mongo::BSONElement e = object.getFieldDotted(VARDATA(arg2));
    if (e.eoo())
    {
        // no such element
        PG_RETURN_NULL();
    }
    else
    {
        return convert_element<FieldType>(fcinfo, e);
    }
}

template<>
Datum
convert_element<std::string>(PG_FUNCTION_ARGS, const mongo::BSONElement e)
{
    try
    {
        return return_string(e.String());
    }
    catch(...)
    {
        PG_RETURN_NULL();
    }
}

// bson manipulation/creation

void composite_to_bson(mongo::BSONObjBuilder& builder, Datum composite);

void datum_to_bson(const char* field_name, mongo::BSONObjBuilder& builder,
    Datum val, bool is_null, Oid typid);

#endif
