#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <deque>
#include <map>
#include <set>
#include <algorithm>
#include <string>
#include <iomanip>
#include <bits/stdc++.h>
using namespace std;

// Instruction states in the pipeline
enum class State {
    IF,  // Fetch
    ID,  // Dispatch
    IS,  // Issue
    EX,  // Execute
    WB   // Writeback
};

// Structure to track timing information for each stage
struct Timing {
    int start;
    int duration;
    
    Timing() : start(0), duration(0) {}
    Timing(int s, int d) : start(s), duration(d) {}
};

// Class to represent a single instruction
class Instruction {
public:
    uint64_t pc;          // Program counter
    int op_type;          // Operation type (0,1,2)
    int dest_reg;         // Destination register
    int src1_reg;         // Source register 1
    int src2_reg;         // Source register 2
    int tag;              // Unique tag for the instruction
    
    State current_state;
    Timing fetch_timing;
    Timing dispatch_timing;
    Timing issue_timing;
    Timing execute_timing;
    Timing writeback_timing;
    
    bool src1_ready;
    bool src2_ready;
    int src1_tag;         // Tag of instruction producing src1
    int src2_tag;         // Tag of instruction producing src2
    int execute_cycles_left;
    
    Instruction(uint64_t pc_, int op_, int dst, int src1, int src2, int tag_)
        : pc(pc_), op_type(op_), dest_reg(dst), src1_reg(src1), src2_reg(src2), tag(tag_),
          current_state(State::IF), src1_ready(false), src2_ready(false), 
          src1_tag(-1), src2_tag(-1), execute_cycles_left(0) {
        // Set execution latency based on operation type
        switch(op_type) {
            case 0: execute_cycles_left = 1; break;
            case 1: execute_cycles_left = 2; break;
            case 2: execute_cycles_left = 10; break;
        }
    }
    
    std::string toString() const {
        char buffer[256];
        snprintf(buffer, sizeof(buffer),
                "%d  fu{%d} src{%d,%d} dst{%d} IF{%d,%d} ID{%d,%d} IS{%d,%d} EX{%d,%d} WB{%d,%d}",
                tag, op_type, src1_reg, src2_reg, dest_reg,
                fetch_timing.start, fetch_timing.duration,
                dispatch_timing.start, dispatch_timing.duration,
                issue_timing.start, issue_timing.duration,
                execute_timing.start, execute_timing.duration,
                writeback_timing.start, writeback_timing.duration);
        return std::string(buffer);
    }
    
    bool isReady() const {
        // Instruction is ready if all source operands are ready
        return (src1_reg == -1 || src1_tag == -1 || src1_ready) &&
               (src2_reg == -1 || src2_tag == -1 || src2_ready);
    }
};

// Class to simulate the processor
class Processor {
private:
    int N;                          // Superscalar width
    int S;                          // Schedule queue size
    static const int ROB_SIZE = 10000;  // Large enough for bookkeeping
    std::deque<Instruction*> ROB;   // Holds all active instructions in program order
    std::vector<Instruction*> dispatch_list;   // Dispatch queue
    std::vector<Instruction*> issue_list;      // Schedule queue
    std::vector<Instruction*> execute_list;    // Function units
    std::map<int, int> register_file;          // Register file (reg_num -> tag mapping)
    std::set<int> completed_tags;              // Set of completed instruction tags
    std::map<int, std::set<int>> pending_reads; // reg_num -> set of tags that need to read it
    int current_cycle;
    int total_instructions;
    
public:
    Processor(int n, int s) : N(n), S(s), current_cycle(0), total_instructions(0) {
        dispatch_list.reserve(2 * N);  // Dispatch queue size is 2N
        issue_list.reserve(S);         // Schedule queue size is S
        execute_list.reserve(N);       // N function units
    }
    
    ~Processor() {
        for (auto instr : ROB) delete instr;
    }

    void fetch(std::vector<Instruction*>& new_instructions) {
        //std::cout << "\nFetch Stage:" << std::endl;
        int fetched = 0;
        while (fetched < N && 
               dispatch_list.size() < 2 * N && 
               !new_instructions.empty()) {
            Instruction* instr = new_instructions.back();
            new_instructions.pop_back();
            instr->current_state = State::IF;
            instr->fetch_timing = Timing(current_cycle, 1);
            
            //std::cout << "Fetching instruction " << instr->tag << std::endl;
            
            ROB.push_back(instr);
            dispatch_list.push_back(instr);
            total_instructions++;
            fetched++;
        }

        for (auto instr : dispatch_list) {
            if (instr->current_state == State::IF && 
                current_cycle > instr->fetch_timing.start) {
                instr->current_state = State::ID;
                //std::cout << "Instruction " << instr->tag << " moving to ID" << std::endl;
            }
        }
    }

    void dispatch() {
    //std::cout << "\nDispatch Stage:" << std::endl;
    
    // Create temp list of instructions in ID state
    std::vector<Instruction*> temp_list;
    for (auto instr : dispatch_list) {
        if (instr->current_state == State::ID) {
            if (instr->dispatch_timing.start > 0) {
                instr->dispatch_timing.duration++;
            }
            temp_list.push_back(instr);
        }
    }

    std::sort(temp_list.begin(), temp_list.end(),
             [](const Instruction* a, const Instruction* b) { return a->tag < b->tag; });

    int dispatched = 0;
    for (auto instr : temp_list) {
        //std::cout << "Trying to dispatch instruction " << instr->tag << std::endl;

        // Check structural hazard (scheduling queue full)
        if (dispatched >= N || issue_list.size() >= S) {
            if (instr->dispatch_timing.start == 0) {
                instr->dispatch_timing = Timing(instr->fetch_timing.start + 
                                              instr->fetch_timing.duration, 1);
            }
            //std::cout << "Instruction " << instr->tag << " stalled due to structural hazard" << std::endl;
            continue;
        }

        // Set initial dispatch timing
        if (instr->dispatch_timing.start == 0) {
            instr->dispatch_timing = Timing(instr->fetch_timing.start + 
                                          instr->fetch_timing.duration, 1);
        }

        // Register renaming - record source tags
        if (instr->src1_reg != -1) {
            auto it = register_file.find(instr->src1_reg);
            if (it != register_file.end()) {
                instr->src1_tag = it->second;
                //std::cout << "Instruction " << instr->tag << " will read R" << instr->src1_reg  << " from tag " << it->second << std::endl;
            } else {
                instr->src1_tag = -1;
            }
        } else {
            instr->src1_tag = -1;
        }

        if (instr->src2_reg != -1) {
            auto it = register_file.find(instr->src2_reg);
            if (it != register_file.end()) {
                instr->src2_tag = it->second;
                //std::cout << "Instruction " << instr->tag << " will read R" << instr->src2_reg  << " from tag " << it->second << std::endl;
            } else {
                instr->src2_tag = -1;
            }
        } else {
            instr->src2_tag = -1;
        }

        // Update register file with destination mapping
        if (instr->dest_reg != -1) {
            register_file[instr->dest_reg] = instr->tag;
            //std::cout << "Mapping reg " << instr->dest_reg << " to tag " << instr->tag << std::endl;
        }

        // Transition to IS state
        instr->current_state = State::IS;
        dispatch_list.erase(std::find(dispatch_list.begin(), dispatch_list.end(), instr));
        issue_list.push_back(instr);
        instr->issue_timing = Timing(instr->dispatch_timing.start + 
                                   instr->dispatch_timing.duration, 1);
        
        //std::cout << "Successfully dispatched instruction " << instr->tag << std::endl;
        dispatched++;
    }
}

void issue() {
    //std::cout << "\nIssue Stage:" << std::endl;
    //std::cout << "Issue list size: " << issue_list.size() << std::endl;
    
    // Print register file state
    //std::cout << "Register File state:" << std::endl;
    for (const auto& reg : register_file) {
        //std::cout << "Reg " << reg.first << " -> Tag " << reg.second << std::endl;
    }

    std::vector<Instruction*> ready_instructions;
    for (auto instr : issue_list) {
        if (instr->current_state == State::IS && 
            current_cycle >= instr->issue_timing.start) {
            
            // Check RAW hazards - Are source operands ready?
            bool src1_ready = (instr->src1_tag == -1) || completed_tags.count(instr->src1_tag) || register_file.find(instr->src1_reg) == register_file.end();
            bool src2_ready = (instr->src2_tag == -1) || completed_tags.count(instr->src2_tag) || register_file.find(instr->src2_reg) == register_file.end();
            
            //std::cout << "Instruction " << instr->tag << " source readiness: "<< "src1=" << src1_ready << " src2=" << src2_ready << "Source1 tags"<<instr->src1_tag<<"Source2 tag"<<instr->src2_tag<<endl;

            // Check WAW hazards - look only at instructions in issue_list
            // that haven't started execution yet
            bool waw_clear = true;
            if (instr->dest_reg != -1) {
                // Check issue_list for instructions that write to same register
                // and haven't been issued yet
                for (auto& other : issue_list) {
                    if (other->tag < instr->tag && 
                        other->dest_reg == instr->dest_reg &&
                        other->current_state == State::IS) {  // Only check instructions not yet in execute
                        waw_clear = false;
                        //std::cout << "Instruction " << instr->tag << " waiting for WAW hazard from tag " << other->tag << " in issue list" << std::endl;
                        break;
                    }
                }
            }
            waw_clear = true;

            if (src1_ready && src2_ready && waw_clear) {
                ready_instructions.push_back(instr);
                //std::cout << "Instruction " << instr->tag << " is ready to issue" << std::endl;
            } else {
                // Only increment duration if instruction is actually stalled due to hazards
                instr->issue_timing.duration++;
                //std::cout << "Instruction " << instr->tag << " stalled due to hazards, increasing duration to "<< instr->issue_timing.duration << std::endl;
            }
        }
    }
    
    // Sort ready instructions by tag
    std::sort(ready_instructions.begin(), ready_instructions.end(),
             [](const Instruction* a, const Instruction* b) { return a->tag < b->tag; });
    
    int issued = 0;
    std::vector<Instruction*> could_not_issue;

    for (auto instr : ready_instructions) {
        // Check structural hazards (superscalar width and function unit availability)
        if (issued >= N) {
            could_not_issue.push_back(instr);
            //std::cout << "Cannot issue instruction " << instr->tag << ": "<< (issued >= N ? "superscalar limit" : "functional units full") << std::endl;
            continue;
        }
        
        //std::cout << "Issuing instruction " << instr->tag << std::endl;
        
        auto it = std::find(issue_list.begin(), issue_list.end(), instr);
        if (it != issue_list.end()) {
            issue_list.erase(it);
        }
        
        instr->execute_timing = Timing(instr->issue_timing.start + 
                                     instr->issue_timing.duration, 
                                     instr->execute_cycles_left);
        
        instr->current_state = State::EX;
        execute_list.push_back(instr);
        issued++;
    }

    // Increment duration for instructions that were ready but couldn't issue
    for (auto instr : could_not_issue) {
        instr->issue_timing.duration++;
        //std::cout << "Instruction " << instr->tag << " was ready but couldn't issue, increasing duration to "<< instr->issue_timing.duration << std::endl;
    }
}

    void execute() {
        //std::cout << "\nExecute Stage:" << std::endl;
        //std::cout << "Execute list size: " << execute_list.size() << std::endl;
        
        for (auto it = execute_list.begin(); it != execute_list.end();) {
            Instruction* instr = *it;
            //std::cout << "Executing instruction " << instr->tag << " cycles left: " << instr->execute_cycles_left << std::endl;
            
            if (current_cycle >= instr->execute_timing.start) {
                instr->execute_cycles_left--;
                
                if (instr->execute_cycles_left == 0) {
                    completed_tags.insert(instr->tag);
                    //std::cout << "Instruction " << instr->tag << " completed execution" << std::endl;
                    instr->current_state = State::WB;
                    instr->writeback_timing = Timing(instr->execute_timing.start + 
                                                   instr->execute_timing.duration, 1);
                    
                    // Wake up dependent instructions
                    for (auto dep : issue_list) {
                        if (dep->src1_tag == instr->tag) {
                            //std::cout << "Waking up instruction " << dep->tag << " src1 dependency on " << instr->tag << std::endl;
                            dep->src1_ready = true;
                        }
                        if (dep->src2_tag == instr->tag) {
                            //std::cout << "Waking up instruction " << dep->tag << " src2 dependency on " << instr->tag << std::endl;
                            dep->src2_ready = true;
                        }
                    }
                    
                    // Update register file
                    if (instr->dest_reg != -1) {
                        auto it = register_file.find(instr->dest_reg);
                        if (it != register_file.end() && it->second == instr->tag) {
                            //std::cout << "Removing mapping for reg " << instr->dest_reg << " (tag " << instr->tag << ")" << std::endl;
                            //register_file.erase(it);
                        }
                    }
                    
                    it = execute_list.erase(it);
                } else {

                    ++it;
                }
            } else {
                ++it;
            }
        }
    }
    
    void retire() {
        //std::cout << "\nRetire Stage:" << std::endl;
        if (!ROB.empty()) {
            //std::cout << "First ROB instruction: Tag " << ROB.front()->tag << " State " << static_cast<int>(ROB.front()->current_state)<< " WB timing: " << ROB.front()->writeback_timing.start << "+" << ROB.front()->writeback_timing.duration << " Current cycle: " << current_cycle << std::endl;
        }
        
        while (!ROB.empty() && 
               ROB.front()->current_state == State::WB &&
               current_cycle >= ROB.front()->writeback_timing.start + ROB.front()->writeback_timing.duration) {
            cout << ROB.front()->toString() << std::endl;
            delete ROB.front();
            ROB.pop_front();
        }
    }
    
    bool advance_cycle() {
        current_cycle++;
        //std::cout << "\n=== Cycle " << current_cycle << " ===" << std::endl;
        
        //std::cout << "ROB size: " << ROB.size() << std::endl;
        if (!ROB.empty()) {
            //std::cout << "First ROB instruction: Tag " << ROB.front()->tag << " State " << static_cast<int>(ROB.front()->current_state) << std::endl;
        }
        
        return !ROB.empty();
    }
    
    void print_configuration() const {
        cout << "CONFIGURATION" << std::endl;
        cout << " superscalar bandwidth (N)      = " << N << std::endl;
        cout << " dispatch queue size (2*N)      = " << 2 * N << std::endl;
        cout << " schedule queue size (S)        = " << S << std::endl;
    }
    
    void print_results() const {
        cout << "RESULTS" << std::endl;
        cout << " number of instructions = " << total_instructions << std::endl;
        cout << " number of cycles       = " << current_cycle-1 << std::endl;
        cout << " IPC                    = " << std::fixed << std::setprecision(2)
                  << static_cast<double>(total_instructions) / current_cycle << std::endl;
    }
};

// Function to read trace file
std::vector<Instruction*> read_trace(const std::string& filename) {
    std::vector<Instruction*> instructions;
    std::ifstream file(filename);
    std::string line;
    int tag = 0;
    
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        uint64_t pc;
        int op_type, dest_reg, src1_reg, src2_reg;
        
        iss >> std::hex >> pc;
        iss >> std::dec >> op_type >> dest_reg >> src1_reg >> src2_reg;
        
        instructions.push_back(new Instruction(pc, op_type, dest_reg, src1_reg, src2_reg, tag++));
        //std::cout << "Read instruction: PC=" << std::hex << pc << std::dec << " op=" << op_type << " dst=" << dest_reg << " src1=" << src1_reg << " src2=" << src2_reg << std::endl;
    }
    
    // Reverse instructions so we can pop from back efficiently
    std::reverse(instructions.begin(), instructions.end());
    return instructions;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <N> <S> <trace_file>" << std::endl;
        return 1;
    }
    
    int N = std::stoi(argv[1]);  // Superscalar width
    int S = std::stoi(argv[2]);  // Schedule queue size
    std::string trace_file = argv[3];
    
    // Read trace file
    std::vector<Instruction*> instructions = read_trace(trace_file);
    
    // Initialize processor
    Processor processor(N, S);
    
    // Main simulation loop
    do {
        processor.retire();
        processor.execute();
        processor.issue();
        processor.dispatch();
        processor.fetch(instructions);
    } while (processor.advance_cycle());
    
    // Print final statistics
    processor.print_configuration();
    processor.print_results();
    
    return 0;
}