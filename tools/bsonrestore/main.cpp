#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/cstdint.hpp>

#include <iostream>
#include <fstream>
#include <sstream>

#include <libpq-fe.h>

#include <stdio.h>

namespace po = boost::program_options;

static void show_help(po::options_description& desc)
{
    std::cout << "bsonrestore - copies data from binary BSON file into PostgreSQL BSON column" << std::endl;
    std::cout << "Usage: " << std::endl;
    std::cout << "    bsonrestore -i data.bson -d mydb -t mycollection -f datafield" << std::endl;
    std::cout << desc << std::endl;
}

static void throw_postgres_error(PGconn* c)
{
    throw std::runtime_error(::PQerrorMessage(c));
}

static void bson_restore(
    const std::string& inputfile,

    const std::string& dbhost,
    int port,
    const std::string& dbname,
    const std::string& table,
    const std::string& field,

    const std::string& username,
    const std::string& password)
{
    // prepare connection
    boost::shared_ptr<PGconn> connection(
        ::PQsetdbLogin(
            dbhost.c_str(),
            boost::lexical_cast<std::string>(port).c_str(),
            NULL,
            NULL,
            dbname.c_str(),
            username.c_str(),
            password.c_str()
        ),
        ::PQfinish);


    if (::PQstatus(connection.get()) != CONNECTION_OK)
    {
        throw_postgres_error(connection.get());
    }


    // open file
    std::ifstream file;
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    file.open(inputfile.c_str());

    // build SQL
    boost::shared_ptr<char> tableEscaped(
        ::PQescapeIdentifier(connection.get(), table.c_str(), table.length()),
        ::PQfreemem);

    boost::shared_ptr<char> fieldEscaped(
        ::PQescapeIdentifier(connection.get(), field.c_str(), field.length()),
        ::PQfreemem);

    std::ostringstream ss;
    ss << "COPY " << tableEscaped.get() << " ( " << fieldEscaped.get() << " ) "
        << " FROM STDIN WITH ( FORMAT \"binary\" );";

    // begin copy oeration
    boost::shared_ptr<PGresult> execRes(
        ::PQexec(connection.get(), ss.str().c_str()),
        ::PQclear);

    if (::PQresultStatus(execRes.get()) != PGRES_COPY_IN)
    {
        throw_postgres_error(connection.get());
    }

    // send header
    const static std::string header =
        "PGCOPY\n\377\r\n\0"    // signature
        "\0\0\0\0"              // flags
        "\0\0\0\0";             // extension

    int res = ::PQputCopyData(connection.get(),
                  header.c_str(),
                  header.length());
    if (res < 0) throw_postgres_error(connection.get());

    // preare buffer
    static const std::size_t BUFSIZE = 16*1024*1024;
    boost::shared_array<char> buffer(new char[BUFSIZE]);

    // let's partition the buffer into fileds
    char* no_of_fields = buffer.get();
    char* field_size = buffer.get()+2;
    char* bson_size = buffer.get()+6;
    char* remainder = buffer.get()+10;

    // no-of-filed is always 1 (big-endian)
    no_of_fields[0] = 0;
    no_of_fields[1] = 1;

    // read file, send tuples
    while(!file.eof())
    {
        // read and verify header
        file.read(bson_size, 4);

        // convert from little endian
        boost::uint8_t* bson_size_as_uchar = reinterpret_cast<boost::uint8_t*>(bson_size);
        boost::uint32_t bson_size_i =
            (bson_size_as_uchar[0]) + (bson_size_as_uchar[1] << 8)+
            (bson_size_as_uchar[2] << 16) + (bson_size_as_uchar[3] << 24);

        if (bson_size_i > BUFSIZE-4)
        {
            throw std::runtime_error("BSON object too big or file corrupted");
        }
        //read the rest
        file.read(remainder, bson_size_i-4);

        // set record size (it's just and inverse bson_size)
        field_size[0] = bson_size[3];
        field_size[1] = bson_size[2];
        field_size[2] = bson_size[1];
        field_size[3] = bson_size[0];

        // send the tuple
        int res = ::PQputCopyData(connection.get(),
                  buffer.get(),
                  bson_size_i + 4
                  );
        if (res < 0) throw_postgres_error(connection.get());
    }

    // finalize copy operation and check the status
    res =  ::PQputCopyEnd(connection.get(), NULL);
    if (res < 0) throw_postgres_error(connection.get());

    boost::shared_ptr<PGresult> copyResult(
        ::PQgetResult(connection.get()),
        ::PQclear);

    if (::PQresultStatus(copyResult.get()) != PGRES_COMMAND_OK)
    {
        throw_postgres_error(connection.get());
    }
}

int main(int argc, char** argv)
{
    char buf[L_cuserid];
    std::string username;
    const char* username_c = ::cuserid(buf);
    if (username_c) username = username_c;



    po::options_description desc("Program options");
    desc.add_options()
        ("inputfile,i", po::value<std::string>(), "Input file")
        ("dbname,d", po::value<std::string>(), "Database name")
        ("table,t", po::value<std::string>(), "Database name")
        ("field,f", po::value<std::string>(), "Field name")
        ("host,h", po::value<std::string>()->default_value("localhost"), "Database server host")
        ("port,p", po::value<int>()->default_value(5432), "Database server port")
        ("username,U", po::value<std::string>()->default_value(username), "Database username")
        ("password", po::value<std::string>()->default_value(""), "Database password")

        ("help", "Show this help")
    ;

    po::variables_map vm;
    try
    {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    }
    catch(const std::exception& e)
    {
        std::cerr << "Invocation error, use --help : " << e.what() << std::endl << std::endl;
        return 1;
    }
    // check options

    if (vm.count("help"))
    {
        show_help(desc);
        return 0;
    }

    if (!vm.count("inputfile") || !vm.count("dbname") || !vm.count("table") || !vm.count("field"))
    {
        std::cerr << "Required param missing, use --help for help" << std::endl;
        return 1;
    }

    try
    {
        bson_restore(
            vm["inputfile"].as<std::string>(),

            vm["host"].as<std::string>(),
            vm["port"].as<int>(),
            vm["dbname"].as<std::string>(),
            vm["table"].as<std::string>(),
            vm["field"].as<std::string>(),

            vm["username"].as<std::string>(),
            vm["password"].as<std::string>()
        );
    }
    catch(const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
