#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <bits/stdc++.h>

// Format checker just assumes you have Alarm.bif and Solved_Alarm.bif (your file) in current directory
using namespace std;

// Our graph consists of a list of nodes where each node is represented as follows:
class Graph_Node{

private:
    string Node_Name;  // Variable name
    vector<int> Children; // Children of a particular node - these are index of nodes in graph.
    vector<string> Parents; // Parents of a particular node- note these are names of parents
    int nvalues;  // Number of categories a variable represented by this node can take - low medium high or true false
    vector<string> values; // Categories of possible values
    vector<float> CPT; // conditional probability table as a 1-d array . Look for BIF format to understand its meaning

public:
    // Constructor- a node is initialised with its name and its categories
    Graph_Node(string name,int n,vector<string> vals)
    {
        Node_Name=name;

        nvalues=n;
        values=vals;

    }
    string get_name()
    {
        return Node_Name;
    }
    vector<int> get_children()
    {
        return Children;
    }
    vector<string> get_Parents()
    {
        return Parents;
    }
    vector<float> get_CPT()
    {
        return CPT;
    }
    int CPTsize() {  // to do
        return CPT.size();
    }
    int CPTindex(vector<int>& values, vector<int>& values_count) {

        int ans = 0, x = 1;
        for (int i = values.size()-1; i>=0; i--) {
            ans += x*values[i];
            x*=values_count[i];
        }
        return ans;
    }
    int get_nvalues()
    {
        return nvalues;
    }
    vector<string> get_values()
    {
        return values;
    }
    void set_CPT(vector<float> new_CPT)
    {
        CPT.clear();
        CPT=new_CPT;
    }
    void set_Parents(vector<string> Parent_Nodes)
    {
        Parents.clear();
        Parents=Parent_Nodes;
    }
    // add another node in a graph as a child of this node
    int add_child(int new_child_index )
    {
        for(int i=0;i<Children.size();i++)
        {
            if(Children[i]==new_child_index)
                return 0;
        }
        Children.push_back(new_child_index);
        return 1;
    }

};


class network {
    list <Graph_Node> Pres_Graph;
    vector<int> qmarksIndex; // stores the index of variable having a ? in the sample; size = # records
    vector<vector<int>> actualData; // size = #records * #variables
    vector<vector<int>> data; // size = #records * #variables + (handling ? marks)
    vector<float> weightSamples; // size = #records + (handling ? marks)
    float smoothingFactor = 0.0008;
    float timeLimit = 119.998;

    public:

    int addNode(Graph_Node node)
    {
        Pres_Graph.push_back(node);
        return 0;
    }


    int netSize()
    {
        return Pres_Graph.size();
    }
    // get the index of node with a given name
    int get_index(string val_name)
    {
        list<Graph_Node>::iterator listIt;
        int count=0;
        for(listIt=Pres_Graph.begin();listIt!=Pres_Graph.end();listIt++)
        {
            if(listIt->get_name().compare(val_name)==0)
                return count;
            count++;
        }
        return -1;
    }
    // get the node at nth index
    list<Graph_Node>::iterator get_nth_node(int n)
    {
       list<Graph_Node>::iterator listIt;
        int count=0;
        for(listIt=Pres_Graph.begin();listIt!=Pres_Graph.end();listIt++)
        {
            if(count==n)
                return listIt;
            count++;
        }
        return listIt;
    }
    //get the iterator of a node with a given name
    list<Graph_Node>::iterator search_node(string val_name)
    {
        list<Graph_Node>::iterator listIt;
        for(listIt=Pres_Graph.begin();listIt!=Pres_Graph.end();listIt++)
        {
            if(listIt->get_name().compare(val_name)==0)
                return listIt;
        }

            cout<<"node not found\n";
        return listIt;
    }

    void readData(string data_file) {
        ifstream myfile(data_file);
        string line;
        string var_value;

        if (!myfile.is_open()) {
            return;
        } else {
            int size = netSize();
            while (! myfile.eof() ) {
                stringstream ss;
                getline (myfile,line);
                ss.str(line);
                vector<int> id_values;
                int qIndex = -1;

                for (int i = 0; i<size; i++) {
                    ss >> var_value;

                    if (var_value.compare("\"?\"") == 0) {
                        qIndex = i;
                        id_values.push_back(-1);

                    } else {
                        
                        list<Graph_Node>::iterator nd = get_nth_node(i);
                        vector<string> psblValues = nd->get_values();
                        int id;
                        for (int j=0; j<psblValues.size(); j++) {
                            if (psblValues[j].compare(var_value) ==0 ) {
                                id = j;
                                break;
                            }
                        }
                        id_values.push_back(id);
                    }

                }

                actualData.push_back(id_values);
                qmarksIndex.push_back(qIndex);
                if (qIndex != -1) {
                    list<Graph_Node>::iterator nd = get_nth_node(qIndex);
                    int s = nd->get_nvalues();
                    vector<string> psblValues = nd->get_values();
                    for (int i = 0; i<s; i++) {
                        id_values[qIndex] = i;
                        data.push_back(id_values);
                        weightSamples.push_back(1.0/s);
                    }
                } else {
                    data.push_back(id_values);
                    weightSamples.push_back(1.0);
                }

            }
            myfile.close();
        }
    }

    void Initializing_CPT(){
        for(int i =0;i<netSize();i++){
            list<Graph_Node>::iterator nd = get_nth_node(i);
            vector<float> tmpCPT = nd->get_CPT();

            for(int p =0;p<nd->CPTsize();p++){

                tmpCPT[p] = 1.0/(nd->get_nvalues());
            }
            nd->set_CPT(tmpCPT);
        }
    }

    
    float getProbability(int i,int qindex, int value){
        vector<int> values;
        vector<int> values_count;

        values.push_back(value);
        list<Graph_Node>::iterator nd = get_nth_node(qindex);
        // int pos_values_size = nd->get_nvalues();
        values_count.push_back(nd->get_nvalues());

        vector<string> p = nd->get_Parents();

        for(int j = 0; j<p.size(); j++) {
            int idx = get_index(p[j]);
            list<Graph_Node>::iterator par = get_nth_node(idx);
            // int pos_values_size = get_nth_node(idx)->get_nvalues();
            values_count.push_back(par->get_nvalues());
            values.push_back(actualData[i][idx]);
        }
        vector<float> tmpCPT = nd->get_CPT();
        return tmpCPT[nd->CPTindex(values, values_count)];

    }

    void updateWeights() {
        weightSamples.clear();

        for(int i = 0; i<actualData.size(); i++) {

            if (qmarksIndex[i] != -1) {
                list<Graph_Node>::iterator nd = get_nth_node(qmarksIndex[i]);
                vector<string> psblValues = nd->get_values();
                vector<float> probabilities;
                float total_prob = 0;
                vector<int> children = nd->get_children();
                for(int j = 0; j<psblValues.size(); j++) {
                    float prob = getProbability(i, qmarksIndex[i], j);
                    for(int k = 0; k<children.size(); k++) {
                        actualData[i][qmarksIndex[i]] = j;
                        int cindex = children[k];
                        float child_prob = getProbability(i, cindex, actualData[i][cindex]);
                        prob*= child_prob;
                    }
                    probabilities.push_back(prob);
                    total_prob += prob;

                }
                int max_index = max_element(probabilities.begin(), probabilities.end()) - probabilities.begin();
                actualData[i][qmarksIndex[i]] = max_index;

                for (int j = 0; j<probabilities.size(); j++) {
                    weightSamples.push_back((probabilities[j]*1.0)/total_prob);
                }
            } else {
                weightSamples.push_back(1.0);
            }

        }

    }

    void updateCPT() {

        int size = netSize();
        list<Graph_Node>::iterator listIt;
        
        int c = 0;
        for(listIt=Pres_Graph.begin();listIt!=Pres_Graph.end();listIt++) {
            // cout << "1\n";
            int CPTsize = listIt->CPTsize();
            vector<float> tmpCPT(CPTsize, smoothingFactor);

            vector<int> var_par; // index of the variable
            vector<int> size_var_par;
            var_par.push_back(c);
            c++;
            int n = listIt->get_nvalues();
            size_var_par.push_back(n);
            vector<string> p = listIt->get_Parents();

            for (int i = 0; i<p.size(); i++) {
                int id = get_index(p[i]);
                var_par.push_back(id);
                list<Graph_Node>::iterator par = get_nth_node(id);
                size_var_par.push_back(par->get_nvalues());
            }

            int parentDist = (CPTsize*1.0)/(n);
            vector<float> sumParents(parentDist, n*smoothingFactor);
            for (int i = 0; i < data.size(); i++) {
                vector<int> values;
                for (int k = 0; k<var_par.size(); k++) {
                    values.push_back(data[i][var_par[k]]);
                }

                int dist_index = listIt->CPTindex(values, size_var_par);
                tmpCPT[dist_index] += weightSamples[i];
                sumParents[dist_index%parentDist] += weightSamples[i];
            }
            for (int i = 0; i<CPTsize; i++) {
                if (sumParents[i%parentDist] == 0) { // to check
                    tmpCPT[i] = 0.0;
                } else {
                    tmpCPT[i] /= (sumParents[i%parentDist]*1.0);
                }
            }
            listIt->set_CPT(tmpCPT);
        }
    }

    void dataFileWriter(string file_alarmbif) {
        ifstream myfile(file_alarmbif);
        ofstream out;
        out.open("solved_alarm.bif");
        string line;
        string word;
        if (!myfile.is_open())
            return;
        else{
            while (! myfile.eof() )
            {
                stringstream ss;
                getline (myfile,line);
                ss.str(line);

                ss>>word;
                if(word.compare("probability")==0){
                    ss>>word;
                    ss>>word;
                    int word_index = get_index(word);
                    out<<line<<endl;
                    getline(myfile,line);
                    out<< "    table ";
                    list<Graph_Node>::iterator nd = get_nth_node(word_index);
                    int cptsize = nd->CPTsize();
                    vector<float> cpt = nd->get_CPT();
                    for(int i = 0; i<cptsize;i++){
                        if(cpt[i]<0.01){
                            out << "0.0100" << " ";
                        }
                        else
                            out << fixed << setprecision(4) << cpt[i] << " ";
                    }
                    out<<";"<<endl;
                }
                else if(line.compare("")!=0){
                    out<<line<<endl;
                }
                else{
                    out<<line;
                }
            }
        }
    }

    void doInference(string file, time_t start) {
        Initializing_CPT();
        updateWeights();

        while(time(NULL) - start < timeLimit) {
            updateCPT();
            if(time(NULL) - start >= timeLimit){
                break;
            }
            updateWeights();
        }
        dataFileWriter(file);
    }

};

network read_network(string file)
{
    network Alarm;
    string line;
    int find=0;
      ifstream myfile(file);
      string temp;
      string name;
      vector<string> values;

    if (myfile.is_open())
    {
        while (! myfile.eof() )
        {
            stringstream ss;
              getline (myfile,line);


              ss.str(line);
             ss>>temp;


             if(temp.compare("variable")==0)
             {

                     ss>>name;
                     getline (myfile,line);

                     stringstream ss2;
                     ss2.str(line);
                     for(int i=0;i<4;i++)
                     {

                         ss2>>temp;


                     }
                     values.clear();
                     while(temp.compare("};")!=0)
                     {
                         values.push_back(temp);

                         ss2>>temp;
                    }
                     Graph_Node new_node(name,values.size(),values);
                     int pos=Alarm.addNode(new_node);


             }
             else if(temp.compare("probability")==0)
             {

                     ss>>temp;
                     ss>>temp;

                    list<Graph_Node>::iterator listIt;
                    list<Graph_Node>::iterator listIt1;
                     listIt=Alarm.search_node(temp);
                    int index=Alarm.get_index(temp);
                    ss>>temp;
                    values.clear();
                     while(temp.compare(")")!=0)
                     {
                        listIt1=Alarm.search_node(temp);
                        listIt1->add_child(index);
                         values.push_back(temp);

                         ss>>temp;

                    }
                    listIt->set_Parents(values);
                    getline (myfile,line);
                     stringstream ss2;

                     ss2.str(line);
                     ss2>> temp;

                     ss2>> temp;

                     vector<float> curr_CPT;
                    string::size_type sz;
                     while(temp.compare(";")!=0)
                     {

                         curr_CPT.push_back(atof(temp.c_str()));

                         ss2>>temp;



                    }

                    listIt->set_CPT(curr_CPT);


             }
            else
            {

            }





        }

        if(find==1)
        myfile.close();
      }

      return Alarm;
}

int main(int argc, char** argv) {
    
    srand(time(NULL));
    network Alarm;
    string input_file = argv[1];
    string data_file = argv[2];
    Alarm=read_network(argv[1]);
    Alarm.readData(argv[2]);
    Alarm.doInference(argv[1], time(NULL));

    return 0;
}