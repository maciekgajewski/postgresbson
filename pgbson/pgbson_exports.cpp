// Copyright (c) 2012 Maciej Gajewski <maciej.gajewski0@gmail.com>
// 
// Permission to use, copy, modify, and distribute this software and its documentation for any purpose, without fee, and without a written agreement is hereby granted,
// provided that the above copyright notice and this paragraph and the following two paragraphs appear in all copies.
// 
// IN NO EVENT SHALL THE AUTHOR BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, 
// ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE AUTHOR HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// THE AUTHOR SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
// THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND THE AUTHOR HAS NO OBLIGATIONS TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

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

// logical comparison
PG_FUNCTION_INFO_V1(bson_compare);
Datum
bson_compare(PG_FUNCTION_ARGS)
{
    bytea* arg0 = GETARG_BSON(0);
    bytea* arg1 = GETARG_BSON(1);
    mongo::BSONObj object0(VARDATA_ANY(arg0));
    mongo::BSONObj object1(VARDATA_ANY(arg1));

    PG_RETURN_INT32(object0.woCompare(object1));
}

// binsry equality
PG_FUNCTION_INFO_V1(bson_binary_equal);
Datum
bson_binary_equal(PG_FUNCTION_ARGS)
{
    bytea* arg0 = GETARG_BSON(0);
    bytea* arg1 = GETARG_BSON(1);
    mongo::BSONObj object0(VARDATA_ANY(arg0));
    mongo::BSONObj object1(VARDATA_ANY(arg1));

    PG_RETURN_BOOL(object0.binaryEqual(object1));
}

// hash
PG_FUNCTION_INFO_V1(bson_hash);
Datum
bson_hash(PG_FUNCTION_ARGS)
{
    bytea* arg0 = GETARG_BSON(0);
    mongo::BSONObj object0(VARDATA_ANY(arg0));

    PG_RETURN_INT32(object0.hash());
}

}
