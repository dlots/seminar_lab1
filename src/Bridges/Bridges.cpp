#include "Bridges.h"
#include "../BucketSort/BucketSort.h"
#include "../RadixSort/RadixSort.h"
#include "../QuickSort/QuickSort.h"

////////////////////////////////////////////////////////////////////////////////
// UTILITY
////////////////////////////////////////////////////////////////////////////////

void parse_adj_list(char* adj_list, uint64_t len)
{
    adj_map.clear();
    int64_t vertex = -1;
    string number("");
    for (size_t i = 0; i < len; ++i)
    {
        char next_char = adj_list[i];
        if (next_char == '(')
        {
            ++vertex;
            adj_map[vertex] = vector<uint64_t>();
        }
        else if ((next_char == ',') || (next_char == ')'))
        {
            if (!number.empty())
            {
                adj_map[vertex].push_back(stoi(number));
                number.clear();
            }
        }
        else if (next_char == '\0')
        {
        }
        else
        {
            number.push_back(next_char);
        }
    }
}

const char* get_result_pointer(string& result_string)
{
    char* result = (char*)malloc(result_string.size() * sizeof(char));
    if (result == NULL)
    {
        exit(1);
    }
    memset(result, 0, result_string.size() * sizeof(char));

    strcpy(result, result_string.c_str());

    return result;
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// DETERMINISTIC BRIDGES
////////////////////////////////////////////////////////////////////////////////

void bridges_determ_dfs(uint64_t vertex)
{
    colors[vertex] = gray;
    history.push_back(vertex);
    entry[vertex] = ++timer;
    for (auto adjacent : adj_map[vertex])
    {
        if (colors[adjacent] == white)
        {
            bridges_determ_dfs(adjacent);
            M[vertex] = min(M[vertex], M[adjacent]);
            if (M[adjacent] > entry[vertex])
            {
                if (vertex < adjacent)
                {
                    bridges_output << "(" << vertex << "," << adjacent << "),";
                }
                else
                {
                    bridges_output << "(" << adjacent << "," << vertex << "),";
                }
            }
        }
        else if ((history.size() < 2) || (history[history.size() - 2] != adjacent))
        {
            M[vertex] = min(M[vertex], entry[adjacent]);
        }
    }
    colors[vertex] = black;
    history.pop_back();
}

const char* cpp_compute_bridges_determ(char* adj_list, uint64_t len)
{
    parse_adj_list(adj_list, len);

    timer = 0;
    history.clear();
    bridges_output.str("");
    colors = vector<color>(adj_map.size(), white);
    M = vector<size_t>(adj_map.size(), ULONG_MAX);
    entry = vector<size_t>(adj_map.size(), 0);

    bridges_output << "[";

    for (const auto vertex : adj_map)
    {
        if (colors[vertex.first] == white)
        {
            bridges_determ_dfs(vertex.first);
        }
    }

    string bridges_string(bridges_output.str());
    bridges_string.pop_back();
    bridges_string.push_back(']');

    cout << bridges_string;

    return get_result_pointer(bridges_string);
}

const char* compute_bridges_determ(char* adj_list, uint64_t len)
{
    cpp_compute_bridges_determ(adj_list, len);
    return const_cast<const char*>(adj_list);
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// SAMPLING FOR RANDOMIZED BRIDGES AND 2-BRIDGES
////////////////////////////////////////////////////////////////////////////////

void sampling_dfs(uint64_t vertex)
{
    colors[vertex] = gray;
    history.push_back(vertex);
    for (auto adjacent : adj_map[vertex])
    {
        if (colors[adjacent] == white)
        {
            sampling_dfs(adjacent);
        }
        else if ((history.size() < 2) || (history[history.size() - 2] != adjacent))
        {
            uint64_t rand = random_number(mersenne);
            samples[vertex][adjacent] = rand;
            samples[adjacent][vertex] = rand;
        }
    }
    colors[vertex] = black;
    history.pop_back();

    if (history.size() > 0)
    {
        uint64_t parent = history[history.size() - 1];
        uint64_t parent_edge_weight = 0;
        for (auto adjacent : adj_map[vertex])
        {
            if (adjacent != parent)
            {
                parent_edge_weight ^= samples[vertex][adjacent];
            }
        }
        samples[vertex][parent] = parent_edge_weight;
        samples[parent][vertex] = parent_edge_weight;
    }
}

void launch_sampling()
{
    colors = vector<color>(adj_map.size(), white);
    history.clear();
    bridges_output.str("");
    random_device rd;
    mersenne = mt19937_64(rd());

    sampling_dfs(0);
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// RANDOMIZED BRIDGES AND 2-BRIDGES
////////////////////////////////////////////////////////////////////////////////

const char* cpp_compute_bridges_rand(char* adj_list, uint64_t len)
{
    parse_adj_list(adj_list, len);
    launch_sampling();

    vector<pair<uint64_t, uint64_t>> bridges;

    for (auto& first_pair : adj_map)
    {
        auto first_key = first_pair.first;
        for (auto second_key : first_pair.second)
        {
            if (find(bridges.begin(), bridges.end(), make_pair(second_key, first_key)) == bridges.end()
                && samples[first_key][second_key] == 0)
            {
                bridges.push_back(make_pair(first_key, second_key));
            }
        }
    }

    bridges_output << "[";
    for (auto& pair : bridges)
    {
        bridges_output << "(" << pair.first << "," << pair.second << "),";
    }
    string* bridges_string = new string(bridges_output.str());
    bridges_string->pop_back();
    bridges_string->push_back(']');

    return bridges_string->c_str();
}

const char* cpp_compute_2bridges_rand(char* adj_list, uint64_t len, int sort)
{
    parse_adj_list(adj_list, len);
    launch_sampling();

    vector<pair<uint64_t, uint64_t>> edges;
    vector<uint64_t> samples_arr;
    for (auto& first_pair : adj_map)
    {
        auto first_key = first_pair.first;
        for (auto second_key : first_pair.second)
        {
            if (find(edges.begin(), edges.end(), make_pair(second_key, first_key)) == edges.end())
            {
                edges.push_back(make_pair(first_key, second_key));
                samples_arr.push_back(samples[first_key][second_key]);
            }
        }
    }

    vector<uint64_t> samples_arr_copy = samples_arr;
    uint64_t* sorted_args;

    switch (sort)
    {
    case radix:
        sorted_args = radix_sort(samples_arr_copy.data(), samples_arr_copy.size());
        break;
    case bucket:
        sorted_args = cpp_bucket_sort(samples_arr_copy.data(), samples_arr_copy.size());
        break;
    case quick:
        sorted_args = cpp_qsort(samples_arr_copy.data(), samples_arr_copy.size());
        break;
    }

    bridges_output << "[";
    size_t cluster_count = 0;
    for (size_t i = 0; i < samples_arr.size(); ++i)
    {
        ++cluster_count;
        if (samples_arr[sorted_args[i]] != samples_arr[sorted_args[i+1]])
        {
            if (cluster_count > 1)
            {
                bridges_output << "[";
                for (size_t j = i - cluster_count + 1; j < i; ++j)
                {
                    bridges_output << "(" << edges[sorted_args[j]].first << "," << edges[sorted_args[j]].second << "),";
                }
                bridges_output << "(" << edges[sorted_args[i]].first << "," << edges[sorted_args[i]].second << ")],";
            }
            cluster_count = 0;
        }
    }
    string two_bridges_string(bridges_output.str());
    two_bridges_string.pop_back();
    two_bridges_string.push_back(']');

    cout << two_bridges_string;

    return get_result_pointer(two_bridges_string);
}

const char* compute_bridges_rand(char* adj_list, uint64_t len)
{
    //return cpp_compute_bridges_rand(adj_list, len);
    return adj_list;
}

const char* compute_2bridges_rand(char* adj_list, uint64_t len, int sort)
{
    //return cpp_compute_2bridges_rand(adj_list, len, sort);
    return adj_list;
}

////////////////////////////////////////////////////////////////////////////////

int main()
{
    std::string test = "()()()()()(63,90)(51)(58)(59)()(63)(43,79)()(54)()()()()()(29,77)()(51)()()(98)(63,94)(83)()(61)(19,81)(58)()()()()()()(66)()(98)()()(93)(11)()(69)(60)(99)()()(55)(6,21)()(72)(13)(50,73)()()(7,30,97)(8)(46)(28,67)()(5,10,25)()(73)(37)(61)()(45)()(95)(53)(55,65)()()()(19)()(11)(90)(29)()(26)()()()()()()(5,80)()()(42)(25)(71)()(58)(24,39)(47)";

    cpp_compute_bridges_determ(const_cast<char*>(test.c_str()), test.size());

    return 0;
}
