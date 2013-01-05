#include "pgbson_internal.hpp"

#include <string>
#include <cstring>

extern "C" {

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

// package version
PG_FUNCTION_INFO_V1(pgbson_version);
Datum pgbson_version(PG_FUNCTION_ARGS)
{
    return return_string("0.5");
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

PG_FUNCTION_INFO_V1(bson_get_text);
Datum
bson_get_text(PG_FUNCTION_ARGS)
{
    return bson_get<std::string>(fcinfo);
}

PG_FUNCTION_INFO_V1(bson_get_as_bson);
Datum
bson_get_as_bson(PG_FUNCTION_ARGS)
{
    bytea* arg = GETARG_BSON(0);
    mongo::BSONObj object(VARDATA_ANY(arg));

    text* arg2 = PG_GETARG_TEXT_P(1);
    std::string field_name(VARDATA(arg2),  VARSIZE(arg2)-VARHDRSZ);

    mongo::BSONElement el = object.getFieldDotted(field_name);
    if (el.eoo())
    {
        PG_RETURN_NULL();
    }
    else if (el.type() == mongo::Object)
    {
        return return_bson(el.embeddedObject());
    }
    else
    {
        // build object with sinle, anonymous field
        mongo::BSONObjBuilder builder;
        builder.appendAs(el, "");
        return return_bson(builder.obj());
    }
}

// Converts composite type to BSON
//
// Code of this function is based on row_to_json
PG_FUNCTION_INFO_V1(row_to_bson);
Datum
row_to_bson(PG_FUNCTION_ARGS)
{
    PGBSON_LOG << "row_to_bson" << PGBSON_ENDL;
    Datum record = PG_GETARG_DATUM(0);
    mongo::BSONObjBuilder builder;

    composite_to_bson(builder, record);

    return return_bson(builder.obj());
}


}
