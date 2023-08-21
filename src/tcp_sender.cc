#include "tcp_sender.hh"
#include "tcp_config.hh"
#include<iostream>
#include <random>

using namespace std;

/* TCPSender constructor (uses a random ISN if none given) */
TCPSender::TCPSender( uint64_t initial_RTO_ms, optional<Wrap32> fixed_isn )
  : isn_( fixed_isn.value_or( Wrap32 { random_device()() } ) ), initial_RTO_ms_( initial_RTO_ms )
{}

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return {outstanding_seqno_};
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return {consecutive_retransmission_times_};
}

optional<TCPSenderMessage> TCPSender::maybe_send()
{
  if(!segment_out_.empty()&&set_syn_){
    TCPSenderMessage segment = std::move(segment_out_.front());
    segment_out_.pop();
    return segment;
  }else{
    return nullopt;
  }
}

void TCPSender::push( Reader& outbound_stream )
{
  // Your code here.
  const uint64_t curr_window_size = window_size_ ? window_size_ : 1;
  while(curr_window_size > outstanding_seqno_){
    TCPSenderMessage msg;
    if(!set_syn_){
      msg.SYN = true;
      set_syn_ = true;
    }
    
    
    uint64_t payload_size 
      = min(TCPConfig::MAX_PAYLOAD_SIZE,curr_window_size - outstanding_seqno_ - msg.SYN);
    
    payload_size = min(payload_size,outbound_stream.bytes_buffered());

    std::string payload = outbound_stream.get_string(payload_size);
    

    // std::string payload = std::string(outbound_stream.peek()).substr(0,payload_size);


    outbound_stream.pop(payload_size);

    
    
    if(outstanding_seg_.empty()){
      RTO_Timeout_ = initial_RTO_ms_;
      timer_ = 0;
    }

    if(!set_fin_&&outbound_stream.is_finished()
      &&payload.size() + outstanding_seqno_ + msg.SYN < curr_window_size){
      set_fin_ = true;
      msg.FIN = true;
    }

    

    msg.payload = Buffer(std::move(payload));
    if(msg.sequence_length()==0){
      return;
    }
    msg.seqno = get_next_seqno();
    
    segment_out_.push(msg);
    outstanding_seqno_ += msg.sequence_length();
    outstanding_seg_.insert(std::make_pair(next_abs_seqno_,msg));
    next_abs_seqno_ += msg.sequence_length();
    if(msg.FIN){
      break;
    }
  }
}

TCPSenderMessage TCPSender::send_empty_message() const
{
  TCPSenderMessage msg;
  msg.seqno = get_next_seqno();
  return {msg};
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  if(msg.ackno.has_value()){
    //only next_abs_seqno bytes had been assembled, outstanding_msg_ remove the flight msg
    uint64_t recv_abs_seqno = msg.ackno.value().unwrap(isn_,next_abs_seqno_);
    if(recv_abs_seqno > next_abs_seqno_){
      return;
    }
    for(auto iter = outstanding_seg_.begin();iter != outstanding_seg_.end();){
      auto &[index,tmsg] = *iter;
      if(index+tmsg.sequence_length() <= recv_abs_seqno){
        outstanding_seqno_ -= tmsg.sequence_length();
        outstanding_seg_.erase(iter);
        RTO_Timeout_ = initial_RTO_ms_;
        iter = outstanding_seg_.begin();
        if(!outstanding_seg_.empty()){
          timer_ = 0;
        }
      }else{
        break;
      }
    }
    consecutive_retransmission_times_ = 0;
  }
  window_size_ = msg.window_size;
}

void TCPSender::tick( const size_t ms_since_last_tick )
{
  std::cout<<timer_<<endl;
  timer_ += ms_since_last_tick;
  std::cout<<timer_<<"   "<<RTO_Timeout_<<endl;
  if(timer_ >= RTO_Timeout_&&outstanding_seg_.begin()!=outstanding_seg_.end()){
    for(auto seg : outstanding_seg_){
      std::cout<<(std::string)seg.second.payload<<endl;
    }
    const auto &[abs_seqno,segment] = *outstanding_seg_.begin();

    if(window_size_ >0){
      RTO_Timeout_ *= 2;//unknow
    }
    
    segment_out_.push(segment);
    
    consecutive_retransmission_times_ ++;
    timer_ = 0;
  }
}
