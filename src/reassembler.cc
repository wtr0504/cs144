#include "reassembler.hh"
#include <iostream>
using namespace std;

uint64_t first_unassemble(Writer &output){
  return output.bytes_pushed();
}
uint64_t first_unaccept(Writer &output){
  return output.bytes_pushed() + output.available_capacity();
}


void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{

  
  if(is_last_substring){
    eof_ = true;
    eof_index_ = first_index + data.size();
  }
  
  if(first_index < first_unassemble(output)){
    
    if(first_index + data.size() <= first_unassemble(output)){
      return ;
    }
    insert(first_unassemble(output),data.substr(first_unassemble(output)-first_index),is_last_substring,output);
    return;
  }
  
  uint64_t old_first_unassemble = first_unassemble(output);
  uint64_t old_first_unaccapted = first_unaccept(output);

  if(first_index == first_unassemble(output)){
    output.push(data);
    reassemble(old_first_unassemble,old_first_unaccapted,output);
  }else{
    insert_to_reassembler(data,first_index,output);
  }
  if(eof_&&first_unassemble(output)==eof_index_){
    data_.clear();
    output.close();
    return ;
  }
}


void Reassembler::reassemble(uint64_t old_first_unassemble,uint64_t old_first_unaccepted,Writer &output){
  string s = string();
  for(auto i = old_first_unassemble;i<old_first_unaccepted;++i){
    
    if(data_.find(i) != data_.end()){
      if(i < first_unassemble(output)){
        data_.erase(i);
        continue;
      }
      if( i == first_unassemble(output) + s.length()){
        data_.at(i);
        s.push_back(data_.at(i));
        data_.erase(i);
      }else{
        break;
      }
    }
  }
  output.push(s);
}


void Reassembler::insert_to_reassembler(std::string data,uint64_t index,Writer &output){

  for(uint64_t i = 0;i<data.length();++i){
    if(index + i < first_unaccept(output)){
      
      data_.insert(std::pair<uint64_t,char>(index+i,data[i]));
    }else{
      continue;
    }
  }
  
}


uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return data_.size();
}
