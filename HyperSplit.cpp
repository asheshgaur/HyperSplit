#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <string>
#include <sstream>
#include <algorithm>

using namespace std;

struct range
{
    uint32_t low;
    uint32_t high;
};

struct Rule
{
    array<range, 5> fields;
    uint32_t priority;
};

typedef array<uint32_t, 5> Packet;
typedef vector<Packet> Packets;
typedef vector<Rule> Rules;


Packets packets;
Rules rules;
uint8_t binth_val;

class HyperSplitTreeNode {
    public:
        bool isLeaf = false;
        vector<uint32_t> ruleIDs;
        HyperSplitTreeNode* leftChild;
        HyperSplitTreeNode* rightChild;
        int splitField;
        uint32_t splitPoint;
        HyperSplitTreeNode();
        ~HyperSplitTreeNode();
        void sortRules();
};
HyperSplitTreeNode::HyperSplitTreeNode() {
    isLeaf = false;
    leftChild = nullptr;
    rightChild = nullptr;
}
HyperSplitTreeNode::~HyperSplitTreeNode() {
    delete leftChild;
    delete rightChild;
}
void HyperSplitTreeNode::sortRules() {
    sort(ruleIDs.begin(), ruleIDs.end(), [](uint32_t a, uint32_t b) {
        return rules[a].priority < rules[b].priority;
    });
}
//Function Prototypes
void readRulesFromFile(string fileName);
void readPacketsFromFile(string fileName);
void createHyperSplitTree(HyperSplitTreeNode* node, vector<uint32_t> ruleIDs, int field, uint32_t start, uint32_t end);
void classify(HyperSplitTreeNode* node, Packet packet, vector<int>& classifiedRules);
void printTree(HyperSplitTreeNode* node);
int main (int argc, char *argv[])
{
    if (argc != 4)
    {
        cout << "Usage: " << argv[0] << " <rule_file> <packet_file> <binth_val>" << endl;
        return 1;
    }
    //Reading rules and packets from files
    readRulesFromFile(argv[1]);
    readPacketsFromFile(argv[2]);
    binth_val = atoi(argv[3]);
    
    //Creating the root node of the HyperSplit tree
    HyperSplitTreeNode* root = new HyperSplitTreeNode();
    //Determining the ruleIDs to create the HyperSplit tree
    vector<uint32_t> ruleIDs;
    for (int i = 0; i < rules.size(); i++) {
        ruleIDs.push_back(i);
    }
    //Timing the HyperSplit tree creation
    clock_t start = clock();
    //Calling the recursive function to create the HyperSplit tree.
    createHyperSplitTree(root, ruleIDs, -1, 0, 0);
    clock_t end = clock();
    double time_taken = double(end - start) / double(CLOCKS_PER_SEC);
    cout << "Time taken to create HyperSplit tree: " << time_taken << " seconds" << endl;
    //Printing the HyperSplit tree
    //printTree(root);
    //Timing the packet classification
    start = clock();
    //Classifying packets
    vector<int> classifiedRules;
    for (int i = 0; i < packets.size(); i++) {
        classify(root, packets[i], classifiedRules);
    }
    end = clock();
    time_taken = double(end - start) / double(CLOCKS_PER_SEC);
    cout << "Time taken to classify packets: " << time_taken << " seconds" << endl;
    //Writing the classified rules to the output file
    ofstream output("output.txt");
    for (int i = 0; i < classifiedRules.size(); i++) {
        output << classifiedRules[i] << endl;
    }
    output.close();
    delete root;
    return 0;
}
void createHyperSplitTree(HyperSplitTreeNode* node, vector<uint32_t> ruleIDs, int split_field, uint32_t start, uint32_t end) {
    //If the number of rules is less than or equal to 1, then the node is a leaf node
    if (ruleIDs.size() <= binth_val) {
        node->isLeaf = true;
        node->ruleIDs = ruleIDs;
        node->sortRules();
        return;
    }
    //If the number of rules is greater than binth, then we split the rules
    //Determining the endpoints of the ranges in all dimension
    array<vector<uint32_t>, 5> endpoints;
    for (int i = 0; i < ruleIDs.size(); i++)
    {
        for (int j = 0; j < 5; j++)
        {
            endpoints[j].push_back(rules[ruleIDs[i]].fields[j].low);
            endpoints[j].push_back(rules[ruleIDs[i]].fields[j].high);
        }
    }
    //Sorting the endpoints in each dimension
    for (int i = 0; i < 5; i++)
    {
        sort(endpoints[i].begin(), endpoints[i].end());
    }
    //Removing duplicates from the endpoints
    for (int i = 0; i < 5; i++)
    {
        endpoints[i].erase(unique(endpoints[i].begin(), endpoints[i].end()), endpoints[i].end());
    }

    //Determining the field with the minimum average weight of all segments
    double min_avg_weight = 1000000000;
    int min_avg_weight_field = -1;
    for (int i = 0; i < 5; i++)
    {
        double avg_weight = 0;
        for (int j = 0; j < endpoints[i].size() - 1; j++)
        {
            double weight = 0;
            for (int k = 0; k < ruleIDs.size(); k++)
            {
                if (rules[ruleIDs[k]].fields[i].low <= endpoints[i][j] && rules[ruleIDs[k]].fields[i].high >= endpoints[i][j + 1])
                {
                    weight++;
                }
            }
            avg_weight += weight;
        }
        avg_weight /= (endpoints[i].size() - 1);
        if (avg_weight < min_avg_weight)
        {
            min_avg_weight = avg_weight;
            min_avg_weight_field = i;
        }
    }
    ///With the minimum weight field found, we split rules by finding the split point by determining the endpoint at which half
    //of the rules intersect with the segments before it and half intersect with the segments after it.
    vector<uint32_t> weights;
    for (int i = 0; i < endpoints[min_avg_weight_field].size() - 2; i++)
    {
        double weight = 0;
        for (int j = 0; j < ruleIDs.size(); j++)
        {
            if (rules[ruleIDs[j]].fields[min_avg_weight_field].low <= endpoints[min_avg_weight_field][i] && rules[ruleIDs[j]].fields[min_avg_weight_field].high >= endpoints[min_avg_weight_field][i + 1])
            {
                weight++;
            }
        }
        weights.push_back(weight);
    }
    //Calculating the total weight and half weight
    uint32_t total_weight = 0;
    for (auto weight : weights)
    {
        total_weight += weight;
    }
    uint32_t half_weight = total_weight / 2;
    //Finding the split point
    uint32_t tempWeight= 0;
    uint32_t split_point = 0;
    for (int i = 0; i < weights.size(); i++)
    {
        if (tempWeight >= half_weight)
        {
            split_point = endpoints[min_avg_weight_field][i];
            //cout << "Split point: " << split_point << endl;
            break;
        }
        else
        {
            tempWeight += weights[i];
        }
    }
    //Checking if the start and end points as well as the field are the same as parent node. If so, then the node is a leaf node
    uint32_t startpoint = endpoints[min_avg_weight_field][0];
    uint32_t endpoint = endpoints[min_avg_weight_field][endpoints[min_avg_weight_field].size() - 1];
    if (start == startpoint && end == endpoint && split_field == min_avg_weight_field)
    {
        node->isLeaf = true;
        node->ruleIDs = ruleIDs;
        node->sortRules();
        return;
    }
    //Creating the root node of the HyperSplit tree
    //HyperSplitTreeNode* root = new HyperSplitTreeNode(min_avg_weight_field, split_point);
    node->splitField = min_avg_weight_field;
    node->splitPoint = split_point;
    
    //determine the ruleIDs that go to the left child and right child
    vector<uint32_t> leftChildRuleIDs;
    vector<uint32_t> rightChildRuleIDs;
    for (int i = 0; i < ruleIDs.size(); i++)
    {
        if (rules[ruleIDs[i]].fields[min_avg_weight_field].high < split_point)
        {
            leftChildRuleIDs.push_back(ruleIDs[i]);
        }
        else if (rules[ruleIDs[i]].fields[min_avg_weight_field].low > split_point)
        {
            rightChildRuleIDs.push_back(ruleIDs[i]);
        }
        else
        {
            leftChildRuleIDs.push_back(ruleIDs[i]);
            rightChildRuleIDs.push_back(ruleIDs[i]);
        }
    }
    node->leftChild = new HyperSplitTreeNode();
    node->rightChild = new HyperSplitTreeNode();
    createHyperSplitTree(node->leftChild, leftChildRuleIDs, min_avg_weight_field, startpoint, endpoint);
    createHyperSplitTree(node->rightChild, rightChildRuleIDs, min_avg_weight_field, startpoint, endpoint);
}
bool ruleMatchesPacket(Rule rule, Packet packet) {

    for (int i = 0; i < 5; i++) {
        if (rule.fields[i].low > packet[i] || rule.fields[i].high < packet[i]) {
            return false;
        }
    }
    return true;
}
void classify (HyperSplitTreeNode* node, Packet packet, vector<int>& classifiedRules) {
    if (node->isLeaf) {
        for (int i = 0; i < node->ruleIDs.size(); i++) {
            if (ruleMatchesPacket(rules[node->ruleIDs[i]], packet)) {
                classifiedRules.push_back(node->ruleIDs[i]);
                return;
            }
        }
        classifiedRules.push_back(-1);
    }
    else {
        if (packet[node->splitField] < node->splitPoint) {
            classify(node->leftChild, packet, classifiedRules);
        }
        else {
            classify(node->rightChild, packet, classifiedRules);
        }
    }
}
void readPacketsFromFile(string fileName) {
	ifstream inputFile(fileName);
	//vector<Packet> packets;
	uint32_t sip, dip, sp, dp, proto;
	//int count = 0;
	while(inputFile >> sip >> dip >> sp >> dp >> proto) {
		Packet packet;
		packet[0] = sip;
		packet[1] = (dip);
		packet[2] = (sp);
		packet[3] = (dp);
		packet[4] = (proto);
		packets.push_back(packet);
	}
    inputFile.close();
}

void readRulesFromFile(string fileName) {
    static uint32_t ruleID = 0;
    ifstream inputFile(fileName);
    //vector<Rule> rules;
    uint32_t sip_low, sip_high, dip_low, dip_high, sp_low, sp_high, dp_low, dp_high, proto_low, proto_high;
    //int count = 0;
    while(inputFile >> sip_low >> sip_high >> dip_low >> dip_high >> sp_low >> sp_high >> dp_low >> dp_high >> proto_low >> proto_high) {
        Rule rule;
        rule.fields[0].low = sip_low;
        rule.fields[0].high = sip_high;
        rule.fields[1].low = dip_low;
        rule.fields[1].high = dip_high;
        rule.fields[2].low = sp_low;
        rule.fields[2].high = sp_high;
        rule.fields[3].low = dp_low;
        rule.fields[3].high = dp_high;
        rule.fields[4].low = proto_low;
        rule.fields[4].high = proto_high;
        rule.priority = ruleID++;
        rules.push_back(rule);
    }
    inputFile.close();
}

void printTree(HyperSplitTreeNode* node) {
    if (node->isLeaf) {
        cout << "Leaf Node" << endl;
        for (int i = 0; i < node->ruleIDs.size(); i++) {
            cout << node->ruleIDs[i] << " ";
        }
        cout << endl;
    }
    else {
        cout << "Non-Leaf Node" << endl;
        cout << "Split Field: " << node->splitField << endl;
        cout << "Split Point: " << node->splitPoint << endl;
        printTree(node->leftChild);
        printTree(node->rightChild);
    }
}