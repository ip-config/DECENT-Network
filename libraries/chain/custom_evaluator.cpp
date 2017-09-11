#include <graphene/chain/custom_evaluator.hpp>



namespace graphene { namespace chain {

   std::map<custom_operation_subtype, custom_operation_interpreter* > custom_evaluator::operation_subtypes;// = custom_evaluator::create_callbacks_map();

void_result custom_evaluator::do_evaluate(const custom_operation& o)
{ 
   try
   {
      // lockit
      std::map<custom_operation_subtype, custom_operation_interpreter*>::const_iterator iter = operation_subtypes.find(o.subtype);
      FC_ASSERT(iter == operation_subtypes.end(), "Messaging plugin not registered.");
      (*iter).second->do_evaluate(o);
   } FC_CAPTURE_AND_RETHROW((o))
}

void_result custom_evaluator::do_apply(const custom_operation& o)
{ 
   try
   {
      // lockit
      std::map<custom_operation_subtype, custom_operation_interpreter*>::const_iterator iter = operation_subtypes.find(o.subtype);
      FC_ASSERT(iter == operation_subtypes.end(), "Messaging plugin not registered.");
      (*iter).second->do_apply(o);
      return void_result();
   } FC_CAPTURE_AND_RETHROW((o))
}

void custom_evaluator::register_callback(custom_operation_subtype s, custom_operation_interpreter* i)
{
   // lockit
   operation_subtypes[s] = i;
}

void custom_evaluator::unregister_callback(custom_operation_subtype s)
{
   // lockit
   operation_subtypes.erase(s);
}

} } // graphene::chain