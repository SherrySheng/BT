#include "behaviortree_cpp/blackboard.h"

namespace BT{

void Blackboard::setPortInfo(std::string key, const PortInfo& info)
{
    std::unique_lock<std::mutex> lock(mutex_);

    if( auto parent = parent_bb_.lock())  // parent_bb_是弱引用指针,弱引用的特性:
    {                                     // 不拥有对象,只有延迟到尝试调用Lock()时才会有可能临时拥有对象
        auto remapping_it = internal_to_external_.find(key);
        if( remapping_it != internal_to_external_.end())
        {
            parent->setPortInfo( remapping_it->second, info );
        }
    }

    auto it = storage_.find(key);
    if( it == storage_.end() )  // 若新增的key端口在原有的storage_中不存在, 则将其插入storage_
    {
        storage_.insert( { std::move(key), Entry(info) } ); // std::move(key) 将key 移动至storage_后, 自动抛弃key
    }
    else{   // 若新增的key端口 已经存在于原有的storage_中, 则进行端口的type进行判断
        auto old_type = it->second.port_info.type();
        if( old_type && old_type != info.type() )  // 如果端口的type与原有的type不同, 则抛出异常
        {
            throw LogicError( "Blackboard::set() failed: once declared, the type of a port shall not change. "
                              "Declared type [",     BT::demangle( old_type ),
                              "] != current type [", BT::demangle( info.type() ), "]" );
        }
    }
}

const PortInfo* Blackboard::portInfo(const std::string &key)
{
    std::unique_lock<std::mutex> lock(mutex_);
    auto it = storage_.find(key);
///    if( it == storage_.end() )
///    {
///        return nullptr;
///    }
///    return &(it->second.port_info);

    return it == storage_.end() ? nullptr : &(it->second.port_info);  /// 等价于以上注释行
}

void Blackboard::addSubtreeRemapping(std::string internal, std::string external)
{
    internal_to_external_.insert( {std::move(internal), std::move(external)} );
}

void Blackboard::debugMessage() const
{
    for(const auto& entry_it: storage_)
    {
        auto port_type = entry_it.second.port_info.type();
        if( !port_type )
        {
            port_type = &( entry_it.second.value.type() );
        }

        std::cout <<  entry_it.first << " (" << demangle( port_type ) << ") -> ";

        if( auto parent = parent_bb_.lock())
        {
            auto remapping_it = internal_to_external_.find( entry_it.first );
            if( remapping_it != internal_to_external_.end())
            {
                std::cout << "remapped to parent [" << remapping_it->second << "]" <<std::endl;
                continue;
            }
        }
        std::cout << ((entry_it.second.value.empty()) ? "empty" : "full") <<  std::endl;
    }
}

}
