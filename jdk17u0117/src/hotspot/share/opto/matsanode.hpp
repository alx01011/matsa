#ifndef MATSA_NODE
#define MATSA_NODE

#include "opto/node.hpp"
#include "opto/callnode.hpp"

#include "matsa/matsaRTL.hpp"

/*
    * loadstore node keeps track of the load/store
    * metadata kept:
    *  - address of the load/store (node)
    *  - method (ciMethod*)
    *  - bci (int)
    *  - size of the load/store (int)
    *  - load or store? (bool)
*/ 


class MaTSaLoadStoreNode : public CallNode {
    public:
    // for now will keep the fields public (change later to comply with the style)
    ciMethod *_m;
    int _bci;
    int _fsize; // field size
    bool _is_store; // is it a store or a load?

    const TypeFunc * matsa_load_store_Type() {
        // void *addr, Method *m, int bci, uint8_t access_size, bool is_write
        const Type **fields = TypeTuple::fields(5);
        fields[TypeFunc::Parms+0] = TypeRawPtr::BOTTOM; // addr
        fields[TypeFunc::Parms+1] = TypeMetadataPtr::BOTTOM; // method we are in
        fields[TypeFunc::Parms+2] = TypeInt::INT; // bci
        fields[TypeFunc::Parms+3] = TypeInt::INT; // access_size
        fields[TypeFunc::Parms+4] = TypeInt::BOOL; // is_write

        const TypeTuple *domain = TypeTuple::make(TypeFunc::Parms+5, fields);

        // create result type (range)
        fields = TypeTuple::fields(0);

        const TypeTuple *range = TypeTuple::make(TypeFunc::Parms+0,fields);

        return TypeFunc::make(domain,range);
    }
    
    MaTSaLoadStoreNode(Compile *C) 
        : CallNode(matsa_load_store_Type(), CAST_FROM_FN_PTR(address, MaTSaRTL::C2MemoryAccess), TypeRawPtr::BOTTOM) {
            // its a macro node
            init_class_id(Class_MaTSaLoadStore);
            init_flags(Flag_is_macro);

            // add the node to the compile's macro nodes
            // C->add_macro_node(this);
        }

        virtual int Opcode() const;
        virtual uint size_of() const { return sizeof (*this); }

        virtual Node* Ideal(PhaseGVN* phase, bool can_reshape) {
            // we do not want to remove this node
            return nullptr;
        }


};

#endif