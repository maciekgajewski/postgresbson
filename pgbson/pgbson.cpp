// mongo
#include "mongo/db/jsobj.h"
#include "mongo/db/json.h"
#ifdef LOG
#undef LOG
#endif

//postgres
extern "C" {
#include "postgres.h"
#include "fmgr.h"

// bson access macros
#define DatumGetBson(X) ((bytea *) PG_DETOAST_DATUM_PACKED(X))
#define GETARG_BSON(n)  DatumGetBson(PG_GETARG_DATUM(n))

}

#include <string>
#include <cstring>

// util
static Datum return_string(const std::string& s)
{
    std::size_t text_size = s.length() + VARHDRSZ;
    text* new_text = (text *) palloc(text_size);
    SET_VARSIZE(new_text, text_size);
    std::memcpy(VARDATA(new_text), s.c_str(), s.length());
    PG_RETURN_TEXT_P(new_text);
}

static Datum return_cstring(const std::string& s)
{
    char* c = (char*) palloc(s.length()+1);
    std::memcpy(c, s.c_str(), s.length()+1); // +1 to copy the null terminator as well

    PG_RETURN_CSTRING(c);
}

static Datum return_bson(const mongo::BSONObj& b)
{
    std::size_t bson_size = b.objsize() + VARHDRSZ;
    bytea* new_bytea = (bytea *) palloc(bson_size);
    SET_VARSIZE(new_bytea, bson_size);
    std::memcpy(VARDATA(new_bytea), b.objdata(), b.objsize());
    PG_RETURN_BYTEA_P(new_bytea);
}
// object inspection
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

extern "C" {

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

// dummy test function
PG_FUNCTION_INFO_V1(pgbson_test);
Datum pgbson_test(PG_FUNCTION_ARGS)
{
    return return_string("OK");
}

// bson output - to json
PG_FUNCTION_INFO_V1(bson_out);
Datum
bson_out(PG_FUNCTION_ARGS)
{
    bytea* arg = GETARG_BSON(0);
    mongo::BSONObj object(VARDATA_ANY(arg));

    std::string json = object.jsonString(); // strict, not-pretty
    return return_cstring(json);
}

// bson input - from json
PG_FUNCTION_INFO_V1(bson_in);
Datum
bson_in(PG_FUNCTION_ARGS)
{
    char* arg = PG_GETARG_CSTRING(0);
    try
    {
        mongo::BSONObj object = mongo::fromjson(arg, NULL);
        // copy to palloc-ed buffer
        return return_bson(object);
    }
    catch(...)
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION), errmsg("invalid input syntax for BSON"))
        );
    }
}


PG_FUNCTION_INFO_V1(bson_get_string);
Datum
bson_get_string(PG_FUNCTION_ARGS)
{
    return bson_get<std::string>(fcinfo);
}



}
