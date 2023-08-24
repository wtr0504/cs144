#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"
#include<iostream>

using namespace std;

// ethernet_address: Ethernet (what ARP calls "hardware") address of the interface
// ip_address: IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( const EthernetAddress& ethernet_address, const Address& ip_address )
  : ethernet_address_( ethernet_address ), ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

// dgram: the IPv4 datagram to be sent
// next_hop: the IP address of the interface to send it to (typically a router or default gateway, but
// may also be another host if directly connected to the same network as the destination)

// Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) by using the
// Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  auto iter = ARP_cache_.find(next_hop.ipv4_numeric());
  
  if(iter != ARP_cache_.end()){
    EthernetFrame new_ethernet_frame;
    new_ethernet_frame.header.type = EthernetHeader::TYPE_IPv4;
    new_ethernet_frame.header.src = ethernet_address_;
    new_ethernet_frame.header.dst = iter->second.ethernetAdress;
    
    new_ethernet_frame.payload = serialize(dgram);
    ethernet_frame_queue.push(std::move(new_ethernet_frame));
  }
  else{
    if(arp_request_.find(next_hop.ipv4_numeric()) == arp_request_.end()){
      ARPMessage arp_msg;
      arp_msg.opcode = ARPMessage::OPCODE_REQUEST;
      arp_msg.sender_ip_address = ip_address_.ipv4_numeric();
      arp_msg.sender_ethernet_address = ethernet_address_;
      arp_msg.target_ethernet_address = { };
      arp_msg.target_ip_address = next_hop.ipv4_numeric();

      EthernetFrame ARP_request;
      ARP_request.header.dst = ETHERNET_BROADCAST;
      ARP_request.header.src = ethernet_address_;
      ARP_request.header.type = EthernetHeader::TYPE_ARP;
      ARP_request.payload = serialize(arp_msg);
      ethernet_frame_queue.push(std::move(ARP_request));

      arp_request_.emplace(make_pair(next_hop.ipv4_numeric(),ARP_REQUEST_DEFAULT_TTL));
    }
    internetdatagram_to_send_.emplace_back(std::move(make_pair(next_hop,dgram)));
  }
}

// frame: the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  if(frame.header.dst != ETHERNET_BROADCAST && frame.header.dst != ethernet_address_){
    return  nullopt;
  }
  if(frame.header.type == EthernetHeader::TYPE_IPv4){
    InternetDatagram internet_dgram ;
    if(not parse(internet_dgram,frame.payload)){
      return nullopt;
    }
    return internet_dgram;
  }
  else if(frame.header.type == EthernetHeader::TYPE_ARP){
    ARPMessage arp_msg;
    std::cout<<"asda"<<endl;
    if(not parse(arp_msg,frame.payload)){
      return nullopt;
    }
    
    if(arp_msg.target_ip_address == ip_address_.ipv4_numeric()){
      //save ARP
      
      EthernetAndTimer ethernet_timer = {arp_msg.sender_ethernet_address,ARP_DEFAULT_TTL};
      ARP_cache_.insert(std::make_pair(arp_msg.sender_ip_address,ethernet_timer));
      //ARP is request
      if(arp_msg.opcode == ARPMessage::OPCODE_REQUEST ){
        std::cout<<"wqrtry"<<endl;
        ARPMessage arp_reply_msg ;
        arp_reply_msg.opcode = ARPMessage::OPCODE_REPLY; 
        arp_reply_msg.sender_ethernet_address = ethernet_address_;
        arp_reply_msg.sender_ip_address = ip_address_.ipv4_numeric();
        arp_reply_msg.target_ethernet_address = arp_msg.sender_ethernet_address;
        arp_reply_msg.target_ip_address = arp_msg.sender_ip_address;
std::cout<<"123325435"<<endl;
        EthernetFrame eth_reply;
        eth_reply.header.dst = arp_msg.sender_ethernet_address;
        eth_reply.header.src = ethernet_address_;
        eth_reply.header.type = EthernetHeader::TYPE_ARP;
        std::cout<<"123325435"<<endl;
        eth_reply.payload = serialize(arp_reply_msg);
        std::cout<<"123325435"<<endl;
        ethernet_frame_queue.push(std::move(eth_reply));
        std::cout<<"123325435"<<endl;
      }

    //ARP is reply
      else if(arp_msg.opcode ==ARPMessage::OPCODE_REPLY){
        
        for(auto inter_dgram = internetdatagram_to_send_.begin();inter_dgram != internetdatagram_to_send_.end();){
          if(inter_dgram->first.ipv4_numeric() == arp_msg.sender_ip_address){
            send_datagram(inter_dgram->second,inter_dgram->first);
            inter_dgram = internetdatagram_to_send_.erase(inter_dgram);
            arp_request_.erase(arp_msg.sender_ip_address);
          }else{
            inter_dgram++;
          }
        }
      }
    }
  }
  return nullopt;
}

// ms_since_last_tick: the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  auto iter = ARP_cache_.begin();
  for(;iter != ARP_cache_.end();){
    if(iter->second.timer <= ms_since_last_tick){
      iter = ARP_cache_.erase(iter);
    }else{
      iter->second.timer -= ms_since_last_tick;
      iter++;
    }
  }
  for(auto &[ipv4_addr,arp_timer] : arp_request_){
    if(arp_timer <= ms_since_last_tick){
      ARPMessage arp_msg;
      arp_msg.opcode = ARPMessage::OPCODE_REQUEST;
      arp_msg.sender_ip_address = ip_address_.ipv4_numeric();
      arp_msg.sender_ethernet_address = ethernet_address_;
      arp_msg.target_ethernet_address = {};
      arp_msg.target_ip_address = ipv4_addr;

      EthernetFrame arp_eth_frame;
      arp_eth_frame.header.dst = ETHERNET_BROADCAST;
      arp_eth_frame.header.src = ethernet_address_;
      arp_eth_frame.header.type = EthernetHeader::TYPE_ARP;
      arp_eth_frame.payload = serialize(arp_msg);
      ethernet_frame_queue.push(arp_eth_frame);

      arp_timer = ARP_REQUEST_DEFAULT_TTL;
    }else{
      arp_timer -= ms_since_last_tick;
    }
  }
}

optional<EthernetFrame> NetworkInterface::maybe_send()
{
  if(!ethernet_frame_queue.empty()){
    EthernetFrame ethernet_frame = std::move(ethernet_frame_queue.front());
    ethernet_frame_queue.pop();
    return ethernet_frame;
  }
  else{
    return nullopt;
  }
}
