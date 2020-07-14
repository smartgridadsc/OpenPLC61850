/*
    Mapper program - map IEC61850 data objects to variable/addresses in OpenPLC Runtime
*/

#include <iostream>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <regex>

#include "pugixml.hpp"

#define DEVICE_DELIM "/"
#define DELIMITER "."

bool hasOutfile;

std::unordered_map<std::string, std::string> do_ln_mapping;
std::unordered_map<std::string, std::string> var_addr_map;

using namespace pugi;

/*
    Trims leading and trailing whitespaces from string
*/
std::string trimWhitespace(std::string line)
{
    int startIndex = line.find_first_not_of(" \n\r\t");
    int endIndex = line.find_last_not_of(" \n\r\t");
    int count = endIndex - startIndex + 1;

    if (startIndex < 0 || endIndex < 0)
    {
        return line;
    }

    return line.substr(startIndex, count);
}


/* 
    Recursive function to extract PLC variables of data attributes from supplied parent node.
    Parent node can be of DataObject or DataAttribute.
*/
void outputVars(xml_node parent, std::string pathstring, std::ostream &out)
{
    if (parent.child("Private"))
    {
        for (xml_node priv_node = parent.child("Private"); priv_node; priv_node = priv_node.next_sibling("Private"))
        {
            std::string newpathstring = pathstring + DELIMITER + priv_node.attribute("name").value();
            outputVars(priv_node, newpathstring, out);
        }
    }
    else if (parent.child("Property"))
    {
        for (xml_node prop_node = parent.child("Property"); prop_node; prop_node = prop_node.next_sibling("Property"))
        {
            std::string prop_name = prop_node.attribute("Name").value();
            std::string prop_value = prop_node.attribute("Value").value();
            if (prop_name.compare("sMonitoringVar") == 0 && !prop_value.empty())
            {
                std::string address = var_addr_map[prop_value];
                if (address.empty())
                {
                    address = "X";
                }
                out << pathstring << " " << address << "\n";
            }
            else if (prop_name.compare("sControlVar") == 0 && !prop_value.empty())
            {
                std::string address = var_addr_map[prop_value];
                if (address.empty())
                {
                    address = "X";
                }
                out << pathstring << " " << address << "\n";
            }
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3 && argc != 4)
    {
        std::cout << "Invalid number of arguments!\n";
        std::cout << "Usage:\nmapper <SCL-filename> <ST-filename> [<output-filename>]\n";
        return 0;
    }

    char *sclfilename = argv[1];
    char *stfilename = argv[2];
    char *outfilename = NULL;
    std::ofstream outfile;
    std::streambuf *buf;

    if (argc == 4)
    {
        outfilename = argv[3];
        hasOutfile = true;

        outfile = std::ofstream(outfilename);
        if (!outfile)
        {
            std::cout << "Failed to open/create output file (" << outfilename << ")!\n";
            return -1;
        }
        buf = outfile.rdbuf();
    }
    else
    {
        hasOutfile = false;
        buf = std::cout.rdbuf();
    }

    //set stream to outfile or std::cout
    std::ostream out(buf);

    // -----------------------------------------------------
    // Create Map of var_name to address
    // -----------------------------------------------------
    std::ifstream stinput(stfilename);
    if (!stinput)
    {
        std::cout << "Failed to open ST file (" << stfilename << ")!\n";
        return -1;
    }

    std::string line;
    bool inVar = false;
    std::regex reg("[a-zA-Z0-9_]+ AT %[A-Z0-9.]+ : [A-Z0]+;");

    while (std::getline(stinput, line))
    {
        line = trimWhitespace(line);

        if (inVar == false)
        {
            if (line.compare("VAR") == 0)
            {
                inVar = true;
            }
            continue;
        }

        if (inVar && std::regex_match(line, reg))
        {
            std::stringstream ss(line);
            std::string token;

            std::getline(ss, token, ' ');
            //std::cout << "Var_name: " << token << "\n";
            std::string varName = token;

            std::getline(ss, token, ' '); //get rid of "AT"
            std::getline(ss, token, ' ');
            //std::cout << "Address: " << token << "\n";
            std::string address = token;

            var_addr_map[varName] = address;
        }

        for (auto it = var_addr_map.begin(); it != var_addr_map.end(); it++)
        {
            std::cout << it->first << " -> " << it->second << "\n";
        }

        if (inVar && line.compare("END_VAR") == 0)
        {
            inVar = false;
            continue;
        }
    }

    stinput.close();
    std::cout << "Successfully parsed ST file\n";

    // -----------------------------------------------------
    // Map IEC61850 datapoints to address
    // -----------------------------------------------------
    xml_document sclfile;
    xml_parse_result result = sclfile.load_file(sclfilename);

    if (result.status == status_file_not_found)
    {
        std::cout << "Failed to open SCL file!\n";
        return -1;
    }
    else if (result.status == status_io_error)
    {
        std::cout << "Failed to read SCL file!\n";
        return -1;
    }
    else if (result.status != status_ok)
    {
        std::cout << "Error parsing SCL file: " << result.status << "!\n";
        return -1;
    }
    else
    {
        std::cout << "Successfully parsed SCL file\n";
    }

    xml_node dtt = sclfile.child("SCL").child("DataTypeTemplates");

    xml_node ied = sclfile.child("SCL").child("IED");
    std::string ied_name = ied.attribute("name").value();

    xml_node ld = ied.child("AccessPoint").child("Server").child("LDevice");
    std::string ld_name = ld.attribute("inst").value();

    //Get map of do_type -> (ln_name + do_name)
    for (xml_node ln_node = dtt.child("LNodeType"); ln_node; ln_node = ln_node.next_sibling("LNodeType"))
    {
        xml_attribute ln_name = ln_node.attribute("id");
        std::string ln_namestring = ln_name.value();

        for (xml_node do_node = ln_node.child("DO"); do_node; do_node = do_node.next_sibling("DO"))
        {
            xml_attribute do_name = do_node.attribute("name");
            xml_attribute do_type = do_node.attribute("type");
            std::string do_namestring = do_name.value();

            do_ln_mapping[do_type.value()] = ied_name + ld_name + DEVICE_DELIM + ln_namestring + DELIMITER + do_namestring;
        }
    }

    //Path to get var names: dtt > dotype > da > private [ > private]* > property (sMonitoringVar/scontrolVar)
    for (xml_node do_node = dtt.child("DOType"); do_node; do_node = do_node.next_sibling("DOType"))
    {
        xml_attribute do_name = do_node.attribute("id");
        std::string do_namestring = do_name.value();

        for (xml_node da_node = do_node.child("DA"); da_node; da_node = da_node.next_sibling("DA"))
        {
            std::string pathstring = do_ln_mapping[do_namestring];
            outputVars(da_node, pathstring, out);
        }
    }

    if (hasOutfile)
    {
        outfile.close();
    }

    std::cout << "Mapping done, exit\n";

    return 0;
}