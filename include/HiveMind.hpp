#ifndef SWAYAM_HIVE_MIND_HPP
#define SWAYAM_HIVE_MIND_HPP

#include <string>
#include <vector>
#include <mutex>
#include <unordered_set>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <sstream>

namespace Swayam {


class HiveMind
{

private:

    std::mutex hive_mutex;

    std::unordered_set<std::string> hash_set;

    std::vector<std::string> internal_accepted_hashes;

    const std::string state_file_path =
        ".hive_state/accepted_hashes.txt";


    HiveMind()
    {
        std::filesystem::create_directories(
            ".hive_state"
        );


        std::ifstream infile(
            state_file_path
        );


        std::string line;


        while(std::getline(infile,line))
        {
            if(!line.empty())
            {
                hash_set.insert(line);

                internal_accepted_hashes.push_back(line);
            }
        }
    }



public:


    HiveMind(const HiveMind&) = delete;


    HiveMind& operator=(const HiveMind&) = delete;



    static HiveMind& instance()
    {
        static HiveMind global_hive;

        return global_hive;
    }



    std::vector<std::string> accepted_hashes()
    {
        std::lock_guard<std::mutex> lock(
            hive_mutex
        );

        return internal_accepted_hashes;
    }



    bool is_known(
        const std::string& hash
    )
    {
        std::lock_guard<std::mutex> lock(
            hive_mutex
        );


        return hash_set.contains(hash);
    }




    void register_mutation_hash(
        const std::string& hash
    )
    {

        std::lock_guard<std::mutex> lock(
            hive_mutex
        );


        if(hash_set.insert(hash).second)
        {

            internal_accepted_hashes.push_back(hash);


            std::ofstream outfile(
                state_file_path,
                std::ios::app
            );


            outfile
                << hash
                << "\n";
        }

    }




    static void awakenNode(
        const std::string& node_id
    )
    {

        std::cout
        << "[HiveMind] Node awakened: "
        << node_id
        << std::endl;

    }





    template<typename T>

    static void synchronize_collective(
        const T& ledger,
        const std::string& sync_dir
    )
    {

        std::filesystem::create_directories(
            sync_dir
        );


        std::ostringstream stream;


        stream << ledger;


        std::cout
        << "[HiveMind] Sync complete. Size: "
        << stream.str().size()
        << std::endl;

    }





    static void broadcastEvolution(
        const std::string& target
    )
    {

        std::cout
        << "[HiveMind] Evolution broadcast: "
        << target
        << std::endl;

    }



};


}


#endif
