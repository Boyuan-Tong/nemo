// -*- C++ -*-                                                                  
////////////////////////////////////////////////////////////////////////////////
///                                                                             
/// \file   inc/body.h                                                          
///                                                                             
/// \author Walter Dehnen                                                       
/// \date   2000-2005                                                           
///                                                                             
/// \brief  contains declarations of class falcON::bodies,                      
///	    class falcON::snapshot and some useful macros.                      
///                                                                             
////////////////////////////////////////////////////////////////////////////////
//                                                                              
// Copyright (C) 2000-2005 Walter Dehnen                                        
//                                                                              
// This program is free software; you can redistribute it and/or modify         
// it under the terms of the GNU General Public License as published by         
// the Free Software Foundation; either version 2 of the License, or (at        
// your option) any later version.                                              
//                                                                              
// This program is distributed in the hope that it will be useful, but          
// WITHOUT ANY WARRANTY; without even the implied warranty of                   
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU            
// General Public License for more details.                                     
//                                                                              
// You should have received a copy of the GNU General Public License            
// along with this program; if not, write to the Free Software                  
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.                    
//                                                                              
////////////////////////////////////////////////////////////////////////////////
#ifndef falcON_included_body_h
#define falcON_included_body_h 1

#ifndef falcON_included_fields_h
#  include <public/fields.h>
#endif
#ifndef falcON_included_memory_h
#  include <public/memory.h>
#endif
////////////////////////////////////////////////////////////////////////////////
/// All public code for this project is in namespace falcON
namespace falcON {
#ifdef falcON_NEMO
  class nemo_in;
  class snap_in;
  class data_in;
  class nemo_out;
  class snap_out;
  class data_out;
#endif
  // ///////////////////////////////////////////////////////////////////////////
  //                                                                            
  // meta programmed sum over array                                             
  //                                                                            
  // ///////////////////////////////////////////////////////////////////////////
  template<int I, int N1> struct __Sum {
    template<typename T> static T sum(const T*X) {
      return X[I]+__Sum<I+1,N1>::sum(X); }
  };
  template<int I> struct __Sum<I,I> {
    template<typename T> static T sum(const T*X) { 
      return X[I]; }
  };
  template<int N, typename T> T sum(const T X[N]) {
    return N? __Sum<0,N-1>::sum(X) : T(0);
  }

  class ebodies;                                   // declared in FAlCONC.cc    

  // ///////////////////////////////////////////////////////////////////////////
  // ///////////////////////////////////////////////////////////////////////////
  //                                                                            
  //  class falcON::bodies                                                      
  //                                                                            
  /// Holds and manages the body data.                                          
  ///                                                                           
  /// Body data are hold in a linked list of bodies::block s, which store the   
  /// data in a structure of array (SoA) form. Sub-type bodies::iterator        
  /// provides access to body data (via its members and friends) mimicking an   
  /// array of structure (AoS) layout. Sub-type bodies::index provides an       
  /// alternative data access via members of bodies class falcON::bodies also   
  /// supports data I/O and adding/removing of data fields and bodies.          
  ///                                                                           
  // ///////////////////////////////////////////////////////////////////////////
  // ///////////////////////////////////////////////////////////////////////////

  class bodies {

    //==========================================================================
    //                                                                          
    // static data                                                              
    //                                                                          
    //==========================================================================
  public:
    enum {
      NbyMaxBits = fieldset::STD,                  // gravity etc: max fields   
      NbyDefBits = fieldset::gravity,              // gravity etc: default      
      NbyNBDBits = NbyMaxBits & fieldset::nemo,    // gravity etc: nemo I/O     
      SPHMaxBits = fieldset::sphmax,               // SPH etc: max fields       
      SPHDefBits = fieldset::sphdef,               // SPH etc: default          
      SPHNBDBits = SPHMaxBits & fieldset::nemo,    // SPH etc: emo I/O          
      MaxBits    = NbyMaxBits | SPHMaxBits,        // maximum fields            
      DefBits    = NbyDefBits | SPHDefBits,        // default fields            
      NBDBits    = NbyNBDBits | SPHNBDBits         // nemo I/O fields           
    };
    class iterator;
    //==========================================================================
    //                                                                          
    // class bodies::block                                                      
    //                                                                          
    /// Holds a block of body data.                                             
    ///                                                                         
    /// This class is not for direct public usage. A block holds the data of    
    /// up to bodies::index::max_bodies bodies of the same bodytype.            
    ///                                                                         
    //==========================================================================
    class block {
      friend class bodies;
      friend class bodies::iterator;
      //------------------------------------------------------------------------
      const unsigned     NALL;                     // # data                    
      const bodytype     TYPE;                     // type of bodies hold       
      unsigned           NBOD;                     // # bodies hold             
      unsigned           NO;                       // this==bodies::BLOCK[NO]   
      unsigned           FIRST;                    // total index of first body 
      void              *DATA[BD_NQUANT];          // pointers to body data     
      block             *NEXT;                     // blocks: in linked list    
      const bodies*const BODS;                     // pointer back to bodies    
      //------------------------------------------------------------------------
      // data access                                                            
      //------------------------------------------------------------------------
    public:
      void      *data_void(fieldbit f)       { return DATA[value(f)]; }
      const void*data_void(fieldbit f) const { return DATA[value(f)]; }
      //------------------------------------------------------------------------
      template<int BIT>
      typename field_traits<BIT>::type* data() const {
	return field_traits<BIT>::array(DATA[BIT]);
      }
      //------------------------------------------------------------------------
      template<int BIT>
      const typename field_traits<BIT>::type* const_data() const {
	return field_traits<BIT>::c_array(DATA[BIT]);
      }
      //------------------------------------------------------------------------
      template<int BIT>
      typename field_traits<BIT>::type&datum(int i) const {
#if defined(DEBUG) || defined(EBUG)
	if(0 == DATA[BIT])
	  falcON_THROW("trying to access non-allocated data in bodies: %c",
		       BD_SQUANT[BIT]);
	if(i < 0 || i >= NBOD)
	  falcON_THROW("index out of range in data access of bodies: %c",
		       BD_SQUANT[BIT]);
#endif
	return data<BIT>()[i];
      }
      //------------------------------------------------------------------------
      template<int BIT>
      typename field_traits<BIT>::type const&const_datum(int i) const {
#if defined(DEBUG) || defined(EBUG)
	if(0 == DATA[BIT])
	  falcON_THROW("trying to access non-allocated data in bodies: %c",
		       BD_SQUANT[BIT]);
	if(i < 0 || i >= NBOD)
	  falcON_THROW("index out of range in data access of bodies: %c",
		       BD_SQUANT[BIT]);
#endif
	return const_data<BIT>()[i];
      }
      //------------------------------------------------------------------------
      flag      &flg(int i)       { return datum<fieldbit::f>(i); }
      flag const&flg(int i) const { return const_datum<fieldbit::f>(i); }
      //------------------------------------------------------------------------
      void set_first(unsigned f) { FIRST = f; }
      //------------------------------------------------------------------------
      bool has_field(fieldbit f) const { return DATA[value(f)] != 0; }
      //------------------------------------------------------------------------
      bool                is_sph   () const { return TYPE.is_sph(); }
      unsigned     const &N_alloc  () const { return NALL; }
      unsigned     const &N_bodies () const { return NBOD; }
      unsigned            N_free   () const { return NALL - NBOD; }
      unsigned     const &my_No    () const { return NO; }
      const bodies*const &my_bodies() const { return BODS; }
      bodytype     const &type     () const { return TYPE; }
      unsigned     const&first     () const { return FIRST; }
      unsigned           end       () const { return FIRST+NBOD; }
      //------------------------------------------------------------------------
      block       *const&next             () const { return NEXT; }
      block       *      next_of_same_type() const {
	return (NEXT && NEXT->TYPE == TYPE)? NEXT : 0;
      }
      //------------------------------------------------------------------------
      // flag manipulations                                                     
      //------------------------------------------------------------------------
      void reset_flags() const;
      void flag_all_as_active() const falcON_THROWING;
      void flag_as_sph() const falcON_THROWING;
      //------------------------------------------------------------------------
      // construction & data allocation                                         
      //------------------------------------------------------------------------
    private:
      void set_data_void(fieldbit f, void*D) {
	if(0 != D  &&  0 != DATA[value(f)])
	  falcON_Warning("over writing pointer to allocated memory");
	DATA[value(f)] = D;
      }
      //------------------------------------------------------------------------
      // copy a single body within the block                                    
      fieldset copy_body  (                           // R: data copied         
			   unsigned,                  // I: from this index     
			   unsigned,                  // I: to   this index     
			   fieldset = fieldset::all); //[I: copy these data]    
      //------------------------------------------------------------------------
      // copy a number of bodies across blocks                                  
      fieldset copy_bodies(                           // R: data copied         
			   const block*,              // I: from this block     
			   unsigned,                  // I:  and this index     
			   unsigned,                  // I: to   our index here 
			   unsigned,                  // I: these many bodies   
			   fieldset = fieldset::all)  //[I: copy these data]    
	falcON_THROWING;
      //------------------------------------------------------------------------
      block();                                     // not implemented           
      block(block const&);                         // not implemented           
      //------------------------------------------------------------------------
    public:
      block(unsigned,                              // I: our No                 
	    unsigned,                              // I: # data to allocate     
	    unsigned,                              // I: # bodies <= ----       
	    unsigned,                              // I: first body's index     
	    bodytype,                              // I: type of bodies to hold 
	    fieldset,                              // I: data to allocate       
	    bodies *) falcON_THROWING;             // I: ^ back to my bodies    
      //------------------------------------------------------------------------
      void link      (block*n) {
	NEXT = n;
      }
      //------------------------------------------------------------------------
      void reset_data(fieldset) const falcON_THROWING;
      void add_field (fieldbit) falcON_THROWING;
      void del_field (fieldbit) falcON_THROWING;
      void add_fields(fieldset) falcON_THROWING;
      void del_fields(fieldset) falcON_THROWING;
      void set_fields(fieldset) falcON_THROWING;
      void remove    ()         falcON_THROWING;
      ~block() falcON_THROWING;
      //------------------------------------------------------------------------
      void skip(unsigned&, int) const falcON_THROWING;
      //------------------------------------------------------------------------
      // copy up to NALL bodies                                                 
      // - we copy only bodies of the same type as hold here                    
      // - we only copy bodies whose flag matches last argument                 
      // - if the block copied is finished, we take its NEXT, starting at i=0   
      // - on return the block pointer is either NULL or together with i they   
      //   give the first body which was not copied because NALL was exceeded   
      // - NBOD is set to the bodies copied                                     
      fieldset copy(                               // R: data copied            
		    const block*&,                 // I: block[s] to copy from  
		    unsigned    &,                 // I: 1st body to copy       
		    fieldset = fieldset::all,      //[I: which data to copy]    
		    int      = 0)                  //[I: flag for copying]      
	falcON_THROWING;
#ifdef falcON_NEMO
      //------------------------------------------------------------------------
      // read from nemo data inputs                                             
    protected:
      void read_data   (data_in &,unsigned,unsigned)          falcON_THROWING;
      void read_posvel (data_in &,unsigned,unsigned,fieldset) falcON_THROWING;
      void write_data  (data_out&,unsigned,unsigned) const    falcON_THROWING;
      void write_potpex(data_out&,unsigned,unsigned) const    falcON_THROWING;
#endif
    };
    //==========================================================================
    //                                                                          
    // sub-type bodies::index                                                   
    //                                                                          
    /// An integer-like identifier for access to body data.                     
    /// class bodies and bodies::block are friends and can access the members   
    /// no() and in() to find the body data                                     
    ///                                                                         
    /// \note                                                                   
    /// An index is not preserved by methods that change the number of bodies   
    ///                                                                         
    //==========================================================================
  public:
    class index {
      friend class bodies;
      friend class bodies::block;
      //------------------------------------------------------------------------
      /// useful constants,                                                     
      /// determines interpretation of index::I                                 
      enum {
	block_bits = 8,                 ///< number of bits used for blocks No. 
	index_bits = 24,                ///< number of bits used for sub-index. 
	max_blocks = 1 << block_bits,   ///< maximum number of blocks.          
	max_bodies = 1 << index_bits,   ///< maximum number of bodies per block.
	index_mask = max_bodies - 1     ///< mask for extracting sub-index.     
      };
      //------------------------------------------------------------------------
      /// data are stored in 32bits.                                            
      unsigned I;
      //------------------------------------------------------------------------
      /// return No of block, encoded in highest index::block_bits bits.        
      unsigned no() const { return I >> index_bits; }
      //------------------------------------------------------------------------
      /// return sub-index, encoded in lowest index::index_bits bits.           
      unsigned in() const { return I &  index_mask; }
    public:
      //------------------------------------------------------------------------
      /// unitialized construction.                                             
      index() {}
      //------------------------------------------------------------------------
      /// copy constructor.                                                     
      index(index const&i) : I(i.I) {}
      //------------------------------------------------------------------------
      /// constructor from block No (b) and body index (n) within block.        
      index(unsigned b, unsigned n) : I (n | b<<index_bits) {}
      //------------------------------------------------------------------------
      /// make a copy.                                                          
      index& operator= (index const&i) { I=i.I; return *this; }
      //------------------------------------------------------------------------
      /// output in format "no:index".                                          
      friend std::ostream& operator<<(std::ostream&o, const index&i) {
	return o << i.no() << ':' << i.in();
      }
    };
    //==========================================================================
    //                                                                          
    // data of class bodies                                                     
    //                                                                          
    //==========================================================================
  private:
    unsigned         NALL[BT_NUM];                 // # bodies allocated        
    unsigned         NBOD[BT_NUM];                 // # bodies in use           
    unsigned         NTOT;                         // total # bodies in use     
    fieldset         BITS;                         // body data allocated       
    unsigned         NBLK;                         // # blocks in use           
    block           *BLOCK[index::max_blocks];     // table: blocks             
    block           *TYPES[BT_NUM];                // table: bodies per bodytype
    block           *FIRST;                        // first block of bodies     
    mutable bool     SRCC;                         // source data changed?      
    mutable bool     SPHC;                         // SPH data changed?         
    const bool       C_FORTRAN;                    // we are used for C/FORTRAN 
    //==========================================================================
  public:
    block   *const&first_block()        const { return FIRST; }
    //==========================================================================
    //                                                                          
    // Functions providing information about the number of bodies               
    //                                                                          
    //==========================================================================
    /// \name Functions providing information about the number of bodies        
    //@{                                                                        
    /// # bodies allocated for a given bodytype.
    unsigned const&N_alloc (bodytype t) const { return NALL[int(t)]; }
    /// total # bodies allocated.
    unsigned       N_alloc ()           const { return sum<BT_NUM>(NALL); }
    /// # bodies in use for a given bodytype.
    unsigned const&N_bodies(bodytype t) const { return NBOD[int(t)]; }
    /// total # bodies in use.
    unsigned const&N_bodies()           const { return NTOT; }
    /// # SPH bodies in use.
    unsigned const&N_sph   ()           const { return NBOD[bodytype::gas]; }
    /// # standard bodies in use.
    unsigned const&N_std   ()           const { return NBOD[bodytype::std]; }
    /// # bodies allocated but not used for a given bodytype.
    unsigned       N_free  (bodytype t) const { return N_alloc(t)-N_bodies(t); }
    /// total # bodies allocated but not used.
    unsigned       N_free  ()           const { return N_alloc() -N_bodies(); }
    //@}
    //--------------------------------------------------------------------------
    bool     const&srce_data_changed()  const { return SRCC; }
    bool     const&sph_data_changed()   const { return SPHC; }
    //==========================================================================
    //                                                                          
    // Functions providing information about body data hold                     
    //                                                                          
    //==========================================================================
    /// \name Functions providing information about body data hold              
    //@{                                                                        
    /// sum of all body data hold.
    fieldset const&all_bits  ()           const { return  BITS; }
    /// do we have none of these data?
    bool           have_not  (fieldset b) const { return !BITS.intersect(b); }
    /// do we have all of these data?
    bool           have_all  (fieldset b) const { return  BITS.contain(b); }
    /// do we have some of these data?
    bool           have_some (fieldset b) const { return  BITS.intersect(b); }
    /// do we have this datum?
    bool           have      (fieldbit f) const { return  BITS.contain(f); }
    //@}
    //--------------------------------------------------------------------------
#define HAVE(BIT,NAME)						\
    bool have_##NAME() const { return BITS.contain(BIT); }
    DEF_NAMED(HAVE)
#undef HAVE
    //==========================================================================
    //                                                                          
    // member methods using index                                               
    //                                                                          
    //==========================================================================
    /// \name non-const data access using bodies::index
    //@{
    /// return lvalue (can be changed)
    template<int BIT>
    typename field_traits<BIT>::type&datum(index i) const {
      return BLOCK[i.no()]->datum<BIT>(i.in());
    }
#define NonConstAcess(BIT,NAME)					\
    field_traits<BIT>::type      &    NAME    (index i) const {	\
      return datum<BIT>(i);					\
    }
    DEF_NAMED(NonConstAcess);
#undef NonConstAcess
    //@}
    //--------------------------------------------------------------------------
    /// \name const data access using bodies::index
    //@{
    /// return const datum
    template<int BIT>
    typename field_traits<BIT>::type const&const_datum(index i) const {
      return BLOCK[i.no()]->const_datum<BIT>(i.in());
    }
#define ConstAcess(BIT,NAME)					\
    field_traits<BIT>::type const&c_##NAME    (index i) const {	\
      return const_datum<BIT>(i);				\
    }
    DEF_NAMED(ConstAcess);
#undef ConstAcess
    //@}
    //--------------------------------------------------------------------------
    /// \name further methods using bodies::index
    //@{
    /// return true if the named or indicated datum is hold
    bool has_field(index i, fieldbit f) const {
      return BLOCK[i.no()] && BLOCK[i.no()]->has_field(f);
    }
#define HasDatum(Bit,Name)					\
    bool has_##Name(index i) const {				\
      return BLOCK[i.no()] && BLOCK[i.no()]->has_field(Bit);	\
    }
    DEF_NAMED(HasDatum)
#undef HasDatum
    //--------------------------------------------------------------------------
    /// is the index a valid one?
    bool is_valid(index i) const {
      return 0 != BLOCK[i.no()]  &&  i.in() < BLOCK[i.no()]->N_bodies();
    }
    //--------------------------------------------------------------------------
    /// index to the first of all bodies
    index first() const { return index(FIRST->my_No(),0); }
    //--------------------------------------------------------------------------
    /// running index of body (between 0 and N-1).
    unsigned bodyindex(index i) const {
      return BLOCK[i.no()]->first() + i.in();
    }
    //--------------------------------------------------------------------------
    /// comparison between indices
    bool is_less(index a, index b) const {
      return  a.no() == b.no() &&  a.in() < b.in()
	||    BLOCK[a.no()]->FIRST < BLOCK[b.no()]->FIRST;
    }
    //--------------------------------------------------------------------------
    /// bodytype of body.
    bodytype const&type(index i) const {
      return BLOCK[i.no()]->type();
    }
    //@}
    //==========================================================================
    //                                                                          
    // sub-type bodies::iterator                                                
    //                                                                          
    /// C++ style iterator over bodies, provides access to body data.           
    ///                                                                         
    /// Use iterator for easiest access to data of bodies; used a lot in falcON;
    /// Obtain object of type iterator from member of bodies;                   
    /// non-const data access via members, const data access via friends.       
    ///                                                                         
    /// The following two examples assign the position of the body referred to  
    /// iterator \c i1 to the body referred to by iterator \c i2                
    /// \code                                                                   
    ///    i2.datum<fieldbit::x>() = const_datum<fieldbit::x>(i1);              
    ///    i2.pos() = pos(i1);                                                  
    /// \endcode                                                                
    /// The first method allows templated manipulations, while the second is    
    /// more human readable.                                                    
    ///                                                                         
    //==========================================================================
    class iterator {
      friend class bodies;
      const block* B;                              // block*: current           
      unsigned     K, N;                           // index : current & end     
      //------------------------------------------------------------------------
      // private types                                                          
      //------------------------------------------------------------------------
      enum {
	MARK_BODY   = 1<<9,                        // use  9th bit of flag      
	NOT_LONGER  = 1<<10,                       // use 10th bit of flag      
	NOT_SHORTER = 1<<11                        // use 11th bit of flag      
      };
      //------------------------------------------------------------------------
      // construction                                                           
      //------------------------------------------------------------------------
      iterator();                                  // not implemented           
      /// construction from pointer to block and index within block             
      explicit 
      iterator(const block*b,
	       unsigned    k=0) : B(b), K(k), N(B? B->N_bodies() : 0u) {}
      //------------------------------------------------------------------------
      // auxiliary                                                              
      //------------------------------------------------------------------------
      void next_block() {
	B = B->next();                             //   get next block          
	K = 0;                                     //   set index to 0          
	N = B? B->N_bodies() : 0;                  //   set end of indices      
      }
      //------------------------------------------------------------------------
    public:
      /// \name access to bodies, block, sub-index, and total index             
      //@{
      /// return const pointer to my falcON::bodies
      const bodies*const&my_bodies() const {
	return B->my_bodies();
      }
      /// return pointer to my bodies::block
      const block *const&my_block () const {
	return B;
      }
      /// return number of my bodies::block
      unsigned const&my_block_No () const {
	return B->my_No();
      }
      /// return running body index (between 0, N-1)
      unsigned my_index() const {
	return B->first() + K;
      }
      /// friend return const pointer to falcON::bodies of iterator
      friend const bodies*const&bodies_of(iterator const&i) {
	return i.my_bodies();
      }
      /// friend return pointer to bodies::block of iterator
      friend const block *const&block_of(iterator const&i) {
	return i.my_block();
      }
      /// friend return number of bodies::block of iterator
      friend unsigned const&block_No(iterator const&i) {
	return i.my_block_No();
      }
      /// friend returning sub-index within block
      friend unsigned const&subindex(iterator const&i) {
	return i.K;
      }
      /// friend returning running body index (between 0, N-1)
      friend unsigned bodyindex(iterator const&i) {
	return i.my_index();
      }
      //@}
      //------------------------------------------------------------------------
      /// conversion to bodies::index                                           
      operator index() const {
	return index(B->my_No(), K);
      }
      //------------------------------------------------------------------------
      /// are we refering to a valid body?
      bool is_valid() const {
	return B != 0;
      }
      //------------------------------------------------------------------------
      /// conversion to bool: are we refering to a valid body?
      operator bool () const {
	return is_valid();
      }
      //------------------------------------------------------------------------
      /// \name comparison operations                                           
      //@{
      /// are we identical to another iterator?
      bool operator== (iterator const&i) const {
	return B==i.B && K==i.K;
      }
      /// are we different from another iterator?
      bool operator!= (iterator const&i) const {
	return B!=i.B || K!=i.K;
      }
      /// are we before another iterator?
      bool operator<  (iterator const&i) const {
	return my_index()< i.my_index();
      }
      /// are we before or equal to another iterator?
      bool operator<= (iterator const&i) const {
	return my_index()<=i.my_index();
      }
      /// are we after another iterator?
      bool operator>  (iterator const&i) const {
	return my_index()> i.my_index();
      }
      /// are we after or equal to another iterator?
      bool operator>= (iterator const&i) const {
	return my_index()>=i.my_index();
      }
      //@}
      //------------------------------------------------------------------------
      /// \name information about data hold                                     
      //@{
      /// has our body the datum indicated?
      friend bool has_field(iterator const&i, fieldbit f) {
	return i.B && i.B->has_field(f);
      }
#define HasDatum(Bit,Name)				\
      friend bool has_##Name(iterator const&i) {	\
	return falcON::has_field(i,Bit);		\
      }
      DEF_NAMED(HasDatum)
#undef HasDatum
      //@}
      //------------------------------------------------------------------------
      /// \name forward iteration                                               
      //@{
      /// pre-fix: get next body (note: there is no post-fix operator++)
      iterator& operator++() {
	++K;                                       // increment subindex        
	if(K == N) next_block();                   // end of block: next block  
	return *this;
      }
      /// jump k bodies forward (or to last valid body)
      iterator& operator+=(unsigned k) {
	while(is_valid() && k) {
	  unsigned s = min(N-K, k);
	  K += s;
	  k -= s;
	  if(K >= N) next_block();
	}
	return *this;
      }
      /// return *this += k
      iterator operator+ (unsigned k) const {
	return iterator(*this) += k;
      }
      //@}
      //------------------------------------------------------------------------
      /// \name public construction and assignment                              
      //@{
      /// copy constructor
      iterator(iterator const&i) : B(i.B), K(i.K), N(i.N) {}
      /// iterator offset by \e offset from \e i
      iterator(iterator const&i,
	       unsigned       offset) : B(i.B), K(i.K), N(i.N) {
	*this += offset;
      }
      /// assignment: make exact copy
      iterator& operator=(iterator const&i) {
	B = i.B;
	K = i.K;
 	N = i.N;
	return *this;
      }
      //@}
      //------------------------------------------------------------------------
      /// \name non-const data access (allows change of data)                   
      //@{
      /// return lvalue (can be changed)
      template<int BIT>
      typename field_traits<BIT>::type &datum() {
#if defined(DEBUG) || defined(EBUG)
	if(!is_valid())
	  falcON_THROW("body::datum<%c>() called on invalid body",
		       BD_SQUANT[BIT]);
#endif
	return B-> template datum<BIT>(K);
      }
      //------------------------------------------------------------------------
#define NonConstAccess(Bit,Name)				\
      field_traits<Bit>::type&Name() {				\
        return B-> datum<Bit>(K);				\
      }
      DEF_NAMED(NonConstAccess);
#undef NonConstAccess
      //@}
      //------------------------------------------------------------------------
      /// \name const data access                                               
      //@{
      /// return const datum
      template<int BIT> friend
      typename field_traits<BIT>::type const&const_datum(iterator const&i) {
#if defined(DEBUG) || defined(EBUG)
	if(! i.is_valid() )
	  falcON_THROW("const_datum<%c>(body) called on invalid body",
		       BD_SQUANT[BIT]);
#endif
	return i.B-> template const_datum<BIT>(i.K);
      }
#define ConstAccess(Bit,Name)						\
      friend field_traits<Bit>::type const&Name(iterator const&i) {	\
        return i.B-> const_datum<Bit>(i.K);				\
      }
      DEF_NAMED(ConstAccess);
#undef ConstAccess
      /// return angular momentum
      friend vect angmom(iterator const&i) {
	return falcON::pos(i) ^ falcON::vel(i); }
      /// return type of body
      friend bodytype const&type(iterator const&i) {
	i.B->type();
      }
      //@}
      //------------------------------------------------------------------------
      /// \name flag manipulations                                              
      //@{
      /// is flag F set?
      bool flag_is_set       (int const&F) const {
	return falcON::flg(*this).is_set(F);
      }
      /// conversion to flag
      operator const flag&   ()            const {
	return falcON::flg(*this);
      }
      /// flag this body as active
      void flag_as_active    () { flg().add    (flag::ACTIVE); }
      /// flag this body as inactive
      void unflag_active     () { flg().un_set (flag::ACTIVE); }
      void flag_as_sticky    () { flg().add    (flag::STICKY); }
      void unflag_sticky     () { flg().un_set (flag::STICKY); }
      /// flag this body as SPH particle
      void flag_as_sph       () { flg().add    (flag::SPH); }
      /// flag this body as non-SPH particle
      void unflag_sph        () { flg().un_set (flag::SPH); }
      /// flag this body for removal by bodies::remove()
      void flag_for_removal  () { flg().add    (flag::REMOVE); }
      /// flag this body for usage with longer time step
      void allow_longer      () { flg().un_set (NOT_LONGER); }
      /// flag this body not to use longer time step
      void forbid_longer     () { flg().add    (NOT_LONGER); }
      /// flag this body for usage with shorter time step
      void allow_shorter     () { flg().un_set (NOT_SHORTER); }
      /// flag this body not to use shorter time step
      void forbid_shorter    () { flg().add    (NOT_SHORTER); }
      /// flag this body as being marked
      void mark              () { flg().add    (MARK_BODY); }
      /// flag this body as not being marked
      void unmark            () { flg().un_set (MARK_BODY); }
      /// flag this body as being specially SPH marked
      void mark_SPH_special  () { flg().add    (flag::SPH_SPECIAL); }
      /// flag this body as not being specially SPH marked
      void unmark_SPH_special() { flg().un_set (flag::SPH_SPECIAL); }
      //@}
      //------------------------------------------------------------------------
      /// \name const boolean flag informations via members                     
      //@{
      /// has this body mass?
      bool is_source     () const { return falcON::mass(*this) != zero; }
      /// is this body allowed to use a longer time step?
      bool may_go_longer () const { return !flag_is_set(NOT_LONGER);}
      /// is this body allowed to use a shorter time step?
      bool may_go_shorter() const { return !flag_is_set(NOT_SHORTER); }
      /// is this body flagged as being marked?
      bool is_marked     () const { return  flag_is_set(MARK_BODY); }
      //@}
      //------------------------------------------------------------------------
      /// \name const boolean flag informations via friends                     
      //@{
      /// friend: has this body mass?
      friend bool is_source     (const iterator&i) { return i.is_source(); }
      /// friend: is this body allowed to use a longer time step?
      friend bool may_go_longer (const iterator&i) { return i.may_go_longer(); }
      /// friend: is this body allowed to use a shorter time step?
      friend bool may_go_shorter(const iterator&i) { return i.may_go_shorter();}
      /// friend: is this body flagged as being marked?
      friend bool is_marked     (const iterator&i) { return i.is_marked(); }
      //@}
      //------------------------------------------------------------------------
      // I/O                                                                    
      //------------------------------------------------------------------------
      /// formatted output: write bodyindex
      friend std::ostream& operator<<(std::ostream&o, const iterator&i) {
	return o << falcON::bodyindex(i);
      }
      //------------------------------------------------------------------------
#ifdef falcON_NEMO
      iterator& read_data (data_in &, unsigned =0) falcON_THROWING;
      iterator& write_data(data_out&, unsigned =0) falcON_THROWING;
    private:
      iterator& write_potpex(data_out&, unsigned =0) falcON_THROWING;
      iterator& read_posvel (data_in &, fieldset, unsigned =0) falcON_THROWING;
#endif
      //------------------------------------------------------------------------
    };
    //==========================================================================
    //                                                                          
    /// \name generate iterators                                                
    /// use these methods to obtain begin and end iterator when looping over    
    /// all or a subset of all bodies                                           
    //@{                                                                        
    //==========================================================================
    /// begin of all bodies
    iterator begin_all_bodies() const { return iterator(FIRST); }
    /// end of all bodies
    iterator end_all_bodies  () const { return iterator(0); }
    /// begin of bodies of given bodytype
    iterator begin_typed_bodies(bodytype t) const {
      return iterator(TYPES[int(t)]);
    }
    /// end of bodies of given bodytype
    iterator end_typed_bodies(bodytype t) const {
      return iterator(TYPES[int(t)] == 0? 0 :
		      int(t)+1 >= BT_NUM? 0 : TYPES[int(t)+1]);
    }
    /// begin of SPH bodies
    iterator begin_sph_bodies() const {
      return begin_typed_bodies(bodytype::gas); }
    /// end of SPH bodies
    iterator end_sph_bodies() const {
      return end_typed_bodies(bodytype::gas); }
    /// begin of standard (non-SPH) bodies
    iterator begin_std_bodies() const {
      return begin_typed_bodies(bodytype::std); }
    /// end of standard (non-SPH) bodies
    iterator end_std_bodies()const {
      return end_typed_bodies(bodytype::std); }
    /// body of a given bodies::index
    iterator bodyIn(index i) const {
      const block*p = BLOCK[i.no()];
      if(p==0) return iterator(0);
      const unsigned in = i.in();
      return in < p->N_bodies()? iterator(p,in) : iterator(0);
    }
    /// body of a given running index in [0, N_bodies()[
    iterator bodyNo(unsigned i) const {
      const block*p = FIRST;
      while(p && i >= p->end()) p = p->next();
      return p? iterator(p, i - p->first()) : iterator(0);
    }
    /// an invalid body (useful as default argument)
    static iterator bodyNil() { return iterator(0); }
    //@}
    //==========================================================================
    //                                                                          
    /// \name construction, destruction, and management of bodies and body data 
    //@{                                                                        
    //==========================================================================
    /// Constructor 1, backward compatible version
    ///
    /// \param Nb (input) total number of bodies
    /// \param Bd (input) body data fields to allocate (default: mxvapfHRVJFC)
    /// \param Ns (input) number of SPH bodies, must be <= Nb
    explicit 
    bodies(unsigned Nb, fieldset Bd=fieldset(DefBits), unsigned Ns=0u)
      falcON_THROWING;
    //--------------------------------------------------------------------------
    /// Constructor 1, new version
    ///
    /// \param N  (input) array with number of bodies per bodytype
    /// \param Bd (input) body data fields to allocate (default: mxvapfHRVJFC)
    explicit 
    bodies(unsigned*N=0, fieldset Bd=fieldset(DefBits))
      falcON_THROWING;
    //--------------------------------------------------------------------------
    /// Constructor 2: copy constructor
    ///
    /// \param B  (input) bodies to copy
    /// \param Bd (input) body data fields to copy
    /// \param C  (input) if non-zero: copy only bodies which have this flag set
    bodies(bodies const&B, fieldset Bd=fieldset::all, int C=0)
      falcON_THROWING;
    //--------------------------------------------------------------------------
    /// Destruction: delete all data
    ~bodies() falcON_THROWING;
    //--------------------------------------------------------------------------
    /// Add an unsupported body data field
    ///
    /// If the body data field \e f is not yet supported, then we allocate body
    /// data indicated by \e f for all bodies which can have it (i.e. a SPH data
    /// field is only allocated for SPH bodies).
    void add_field (fieldbit f) falcON_THROWING;
    //--------------------------------------------------------------------------
    /// Add unsupported body data fields
    ///
    /// Those body data fields in \e b which are not yet supported are allocated
    /// for all bodies which can have it (i.e. SPH data fields are only
    /// allocated for SPH bodies)
    void add_fields(fieldset b) falcON_THROWING;
    //--------------------------------------------------------------------------
    /// Delete a supported body data field
    ///
    /// If the body data field \e f is presently allocated, it is deleted
    void del_field (fieldbit f) falcON_THROWING;
    //--------------------------------------------------------------------------
    /// Delete supported body data fields
    ///
    /// Those body data fields in \e b which are presently allocated will be
    /// deleted. Fields not allocated are not affected
    void del_fields(fieldset b) falcON_THROWING;
    //--------------------------------------------------------------------------
    /// Resets N, data: equivalent to destructor followed by constructor 1
    /// (new version)
    ///
    /// \param N  (input) array with number of bodies per bodytype
    /// \param Bd (input) body data fields to allocate (default: mxvapfHRVJFC)
    void reset(unsigned*N =0, fieldset Bd= fieldset(DefBits))
      falcON_THROWING;
    //--------------------------------------------------------------------------
    /// Resets N, data: equivalent destructor followed by constructor 1 (old
    /// version)
    ///
    /// \param Nb (input) total number of bodies
    /// \param Bd (input) body data fields to allocate (default: mxvapfHRVJFC)
    /// \param Ns (input) number of SPH bodies, must be <= Nb
    void reset(unsigned Nb, fieldset Bd=fieldset(DefBits), unsigned Ns=0u)
      falcON_THROWING
    {
      unsigned n[BT_NUM] = {Ns, Nb>Ns?Nb-Ns:0};
      reset(n,Bd);
    }
    //--------------------------------------------------------------------------
    /// Resets N, keeps data the same (N[] = bodies per bodytype)
    ///
    /// \param N  (input) array with number of bodies per bodytype
    void resetN(unsigned* N=0) falcON_THROWING
    {
      reset(N,BITS);
    }
    //--------------------------------------------------------------------------
    /// Resets N, keeps data the same
    ///
    /// \param Nb (input) total number of bodies
    /// \param Ns (input) number of SPH bodies, must be <= Nb
    void resetN(unsigned Nb, unsigned Ns= 0u) falcON_THROWING
    {
      reset(Nb,BITS,Ns);
    }
    //--------------------------------------------------------------------------
    /// Reset some body data to zero
    ///
    /// Body data of fields in b, which are allocated, are set to zero
    /// \param b  (input) body data fields to be reset
    void reset_data(fieldset b) const falcON_THROWING {
      for(const block*p=FIRST; p; p=p->next()) p->reset_data(b);
    }
    //@}
    //==========================================================================
    //                                                                          
    /// \name methods using another set of bodies                               
    //@{                                                                        
    //==========================================================================
    /// Copy: replace our data (if any) with those of other bodies (\b not \b 
    /// yet \b implemented)
    ///
    /// We only copy data specified by 2nd argument and copy only those bodies
    /// whose flag matches 3rd argument. Equivalent to (but more efficient than)
    /// destruction followed by constructor 2.
    ///
    /// \note \b NOT \b YET \b IMPLEMENTED
    ///
    /// \param B  (input) bodies to be copied
    /// \param Bd (input, optional) body data to be copied
    /// \param f  (input, optional) flag for bodies to be copied
    /// \note \b not \b yet \b implemented!
    void copy(bodies const&B, fieldset Bd=fieldset::all, int f=0)
      falcON_THROWING;
    //--------------------------------------------------------------------------
    /// Add: add other bodies (\b not \b yet \b implemented)
    /// 
    /// We only copy data specified by 2nd argument and copy only those bodies
    /// whose flag matches 3rd argument. Previously existing bodies are
    /// preserved.
    ///
    /// \note \b NOT \b YET \b IMPLEMENTED
    ///
    /// \param B  (input) bodies to be added
    /// \param Bd (input, optional) body data to be copied
    /// \param f  (input, optional) flag for bodies to be copied
    /// \note \b not \b yet \b implemented!
    void add(bodies const&B,  fieldset Bd=fieldset::all, int f=0)
      falcON_THROWING;
    //--------------------------------------------------------------------------
    /// Merge: merge two sets of bodies
    ///
    /// The bodies merged in will be emptied. Body data not previously supported
    /// by this will be deleted before merging. Body data previously supported
    /// by this but not by that will be allocated but remain unitialized.
    ///
    /// \param B (input/output) bodies to merge, will be empty after call
    void merge(bodies&B) falcON_THROWING;
    //@}
    //==========================================================================
    //                                                                          
    /// \name removing and creation of bodies                                   
    //@{                                                                        
    //==========================================================================
    /// Remove bodies which have been flagged for removal, eg by
    /// iterator::flag_for_removal().
    ///
    /// We will fill in the gaps left by the removed bodies by bodies taken
    /// from the end. This implies that the order of bodies is altered (however,
    /// bodies of the same bodytype are kept together) and that the number of
    /// bodies used is less than that allocated. Use bodies::shrink() to
    /// reduce this memory overhead.
    /// \note alters to body order.
    void remove() falcON_THROWING; 
    //--------------------------------------------------------------------------
    /// Create \e N new bodies of bodytype \e t, which can then be activated
    /// using new_body()
    ///
    /// Will allocate a new block of bodies of given bodytype. However, the
    /// number of bodies used remains unchanged. In order to activate the new
    /// bodies, you must access each individual one using new_body(), which
    /// also allows to set up its data.
    ///
    /// \note bodies created are always made the first of their type
    ///
    /// \param N (input) number of bodies to allocate
    /// \param t (input) type of bodies to allocate
    void create(unsigned N, bodytype t) falcON_THROWING;
    //--------------------------------------------------------------------------
    /// make a body available which is allocated but not currently used.
    ///
    /// If more bodies of type \e t are currently allocated than used, we will
    /// return a bodies::iterator to one of these and record it as being used.
    /// Otherwise (no body available), we will issue a warning an return an
    /// bodies::invalid iterator. To obtain the number of bodies that can be
    /// activated use bodies::N_free().
    ///
    /// \return a bodies::iterator to a new body or an invalid bodies::iterator
    /// \param  t (input) type of body requested
    iterator new_body(bodytype t) falcON_THROWING;
    //--------------------------------------------------------------------------
    /// make a new body of type \e t; if none available, allocate a block of
    /// \e N new bodies first.
    ///
    /// This simple routines combines N_free(), create(), and new_body() via
    /// \code
    ///   if(0 == N_free(t)) create(N,t);
    ///   return new_body(t);
    /// \endcode
    /// \return a valid bodies::iterator to a body of type \e t
    /// \param t (input) bodytype of body requested
    /// \param N (input) if no body available, allocate this many
    iterator new_body(bodytype t, unsigned N) falcON_THROWING
    {
      if(0 == N_free(t)) create(N,t);
      return new_body(t);
    }
    //--------------------------------------------------------------------------
    /// Shrink allocation to actual usage (\b not \b yet \b implemented)
    ///
    /// If more body data are allocated than used, we re-allocate and copy the 
    /// data to avoid that overhead. Even if the allocation equals the usage, 
    /// we may re-allocate the data to group the bodies in the lowest possible
    /// number of blocks.
    /// 
    /// \note \b NOT \b YET \b IMPLEMENTED 
    ///
    void shrink() falcON_THROWING; 
    //@}
    //==========================================================================
    //                                                                          
    /// \name I/O to file                                                       
    //@{                                                                        
    //                                                                          
    //==========================================================================
#ifdef falcON_NEMO
    //--------------------------------------------------------------------------
    /// Read a single NEMO snapshot
    ///
    /// We start reading into the body given by the 3rd argument. There must
    /// be enough space allocated to accomodate input (we do not reset
    /// N). Body data fields are added, if required, by calls to
    /// bodies::add_field().
    ///
    /// \note this routine is used from snapshot::read_nemo()
    ///
    /// \return       body data fields that have been read
    /// \param Si     (input) snapshot input to read from
    /// \param Bd     (input) body data fields to be read
    /// \param start  (input) start position for reading
    /// \param Nread  (input,optional) read this many (default: all in input)
    /// \param warn   (input,optional) issue falcON::warning() for missing data
  protected:
    fieldset read_snapshot(snap_in  const&Si,
			   fieldset       Bd,
			   iterator const&start,
			   unsigned       Nread= 0,
			   bool           warn= 1) falcON_THROWING;
    //--------------------------------------------------------------------------
    /// Write a single NEMO snapshot
    ///
    /// We start writing from the body given by the 3rd argument. The snapshot
    /// output must accomodate the required number of body to write.
    ///
    /// \note this routine is used from snapshot::write_nemo()
    ///
    /// \param So     (input) snapshot output to write to
    /// \param Bd     (input) body data fields to be written
    /// \param start  (input) start position for writint
    /// \param Nwrite (input,optional) write this many (default: all in output)
    void write_snapshot(snap_out const&So,
			fieldset       Bd,
			iterator const&start,
			unsigned       Nwrite= 0) const falcON_THROWING;
#endif // falcON_NEMO
  public:
    //--------------------------------------------------------------------------
    /// Read simple ascii formatted input                                       
    ///
    /// Read data from an ASCII file. Any lines starting with '\#' are ignored.
    /// the number of lines must match (or exceed) the 4th (and 5th) argument.
    /// The order and number of entries in each line must match the 2nd and 3rd
    /// argument. SPH data are assumed present in the first Ns (5th arg) lines
    /// only.
    ///
    /// \param In (input) input stream to read from
    /// \param Bd (input) array of body data fields to read in given order
    /// \param Nd (input) number of entries in that array
    /// \param Nb (input) number of lines to read
    /// \param Ns (input) number of lines with SPH bodies ( <Nb )
    void read_simple_ascii(std::istream  &In,
			   const fieldbit*Bd,			   
			   unsigned       Nd,
			   unsigned       Nb,
			   unsigned       Ns= 0);
    //@}
    //==========================================================================
    //                                                                          
    /// \name miscellaneous                                                     
    //@{                                                                        
    //==========================================================================
    /// set body keys to first+i, i=0...Nbody-1
    void reset_keys(int k=0) {
      add_fields(fieldset::k);
      for(iterator b = begin_all_bodies(); b; ++b,++k) b.key() = k;
    }
    //--------------------------------------------------------------------------
    /// reset body flags
    void reset_flags() const {
      if(BITS.contain(fieldbit::f))
	for(const block* p=FIRST; p; p=p->next())
	  p->reset_flags();
    }
    //--------------------------------------------------------------------------
    void mark_sph_data_read    () const { SPHC = 0; }
    void mark_sph_data_changed () const { SPHC = 1; }
    //--------------------------------------------------------------------------
    void mark_srce_data_read   () const { SRCC = 0; }
    void mark_srce_data_changed() const { SRCC = 1; }
    //--------------------------------------------------------------------------
    /// return the total mass in bodies of given type
    real TotalMass(bodytype) const;
    /// return to total mass bodies of all types
    real TotalMass() const {
      real M(zero);
      for(bodytype t; t; ++t)
	M += TotalMass(t);
      return M;
    }
    //@}
    //==========================================================================
    //                                                                          
    /// \name creating sorted index tables                                      
    //@{                                                                        
    //==========================================================================
    /// Create an index table sorted in func(body) for a subset of all bodies
    ///
    /// \param T     (output)  table of bodies::index, sorted
    /// \param func  (input)   function for property to be sorted
    /// \param start (input)   only consider bodies starting here
    /// \param Nb    (input)   only consider this many bodies (default: all)
    void sorted(Array<index>&T,
		real       (*func)(iterator const&),
		iterator     start,
		unsigned     Nb= 0) const falcON_THROWING;
    //--------------------------------------------------------------------------
    /// Create an index table sorted in func(body) for a subset of all bodies
    /// and also generate a sorted table of quantities
    ///
    /// \param T     (output)  table of bodies::index, sorted
    /// \param Q     (output)  table of quantity, sorted
    /// \param func  (input)   function for property to be sorted
    /// \param start (input)   only consider bodies starting here
    /// \param Nb    (input)   only consider this many bodies (default: all)
    void sorted(Array<index>&T,
		Array<real> &Q,
		real       (*func)(iterator const&),
		iterator     start,
		unsigned     Nb= 0) const falcON_THROWING;
    //--------------------------------------------------------------------------
    /// Create an index table sorted in func(body) for all bodies
    ///
    /// \param T     (output)  table of bodies::index, sorted
    /// \param func  (input)   function for property to be sorted
    void sorted(Array<index>&T,
		real       (*func)(iterator const&)) const falcON_THROWING
    {
      sorted(T,func,begin_all_bodies());
    }
    //--------------------------------------------------------------------------
    /// Create an index table sorted in func(body) for all bodies
    /// and also generate a sorted table of quantities
    ///
    /// \param T     (output)  table of bodies::index, sorted
    /// \param Q     (output)  table of quantity, sorted
    /// \param func  (input)   function for property to be sorted
    void sorted(Array<index>&T,
		Array<real> &Q,
		real       (*func)(iterator const&)) const falcON_THROWING
    {
      sorted(T,Q,func,begin_all_bodies());
    }
    //@}
    //--------------------------------------------------------------------------
  protected:
    //==========================================================================
    //                                                                          
    // protected member methods used for ebodies                                
    //                                                                          
    //==========================================================================
    bodies(char, const unsigned[BT_NUM]) falcON_THROWING;
    void reset(char, fieldbit, void*) falcON_THROWING;
  private:
    //==========================================================================
    //                                                                          
    // private member methods                                                   
    //                                                                          
    //==========================================================================
    // set up blocks to hold N[t] bodies of type t                              
    void set_data(unsigned*) falcON_THROWING;
    //--------------------------------------------------------------------------
    // link the TYPES[] lists together and set FIRST                            
    void link_blocks();
    //--------------------------------------------------------------------------
    // set blocks' FIRST entries                                                
    void set_firsts();
    //--------------------------------------------------------------------------
    // delete all blocks and reset related data                                 
    void del_data() falcON_THROWING;
    //==========================================================================
  };
  falcON_TRAITS(bodies::index,"bodies::index");

  // ///////////////////////////////////////////////////////////////////////////
  // ///////////////////////////////////////////////////////////////////////////
  //                                                                            
  // falcON::body                                                               
  //                                                                            
  /// a body is simply synonymous to bodies::iterator                           
  //                                                                            
  // ///////////////////////////////////////////////////////////////////////////
  // ///////////////////////////////////////////////////////////////////////////
  typedef bodies::iterator body;

  // ///////////////////////////////////////////////////////////////////////////
  // ///////////////////////////////////////////////////////////////////////////
  //                                                                            
  //  class falcON:snapshot                                                     
  //                                                                            
  /// Holds and manages snapshot data (= body data + time)                      
  ///                                                                           
  /// Derived from class falcON::bodies, inheriting all public members.         
  /// In addition, methods for snapshot I/O to file are provided.               
  ///                                                                           
  // ///////////////////////////////////////////////////////////////////////////
  // ///////////////////////////////////////////////////////////////////////////
  class snapshot : public bodies
  {
    bool           INIT;
    double         TINI;
    mutable double TIME;
    //--------------------------------------------------------------------------
  public:
    //==========================================================================
    /// \name initial-time information and manipulation                         
    //@{
    bool const&has_initial_time() const { return INIT; }  ///< has initial time?
    double const&initial_time() const { return TINI; }    ///< get initial time
    void init_time(double t) {                            ///< set initial time
      INIT = true;
      TINI = t;
    }
    //@}
    //==========================================================================
    /// \name time information and manipulation                                 
    //@{
    double const&time() const { return TIME; }            ///< get time
    void advance_time_by(double d) const { TIME += d; }   ///< advance time
    void set_time(double t) const { TIME  = t; }          ///< set time
    void reset_time() const { TIME  = 0.; }               ///< set time to zero
    //@}
    //==========================================================================
    /// \name construction & related                                            
    //@{
    //--------------------------------------------------------------------------
    /// Constructor 0: just give the fields to be supported,
    /// used in NBodyCode::NBodyCode() of file nbody.h
    explicit
    snapshot(fieldset Bd= fieldset(DefBits)) falcON_THROWING
    : bodies ( static_cast<unsigned*>(0), Bd ),
      INIT   ( false ),
      TINI   ( 0. ),
      TIME   ( 0. ) {}
    //--------------------------------------------------------------------------
    /// Constructor 1 (old version)
    ///
    /// \param t  (input) time of snapshot
    /// \param Nb (input) total number of bodies
    /// \param Bd (input, optional) body data fields to be allocated
    /// \param Ns (input, optional) number of SPH bodies
    snapshot(double   t,
	     unsigned Nb,
	     fieldset Bd = fieldset(DefBits),
	     unsigned Ns = 0) falcON_THROWING
    : bodies ( Nb,Bd,Ns ),
      INIT   ( true ),
      TINI   ( t ),
      TIME   ( t ) {}
    //--------------------------------------------------------------------------
    /// Constructor 1 (new version)
    ///
    /// \param t  (input, optional) time of snapshot
    /// \param N  (input, optional) number of bodies per bodytype
    /// \param Bd (input, optional) body data fields to be allocated
    explicit 
    snapshot(double   t,
	     unsigned*N = 0 ,
	     fieldset Bd= fieldset(DefBits)) falcON_THROWING
    : bodies ( N,Bd ),
      INIT   ( true ),
      TINI   ( t ),
      TIME   ( t ) {}
    //--------------------------------------------------------------------------
    /// copy constructor from bodies
    ///
    /// make a partial copy, only copying data specified by 3rd arg
    ///
    /// \param t  (input) time of snapshot
    /// \param B  (input) set of bodies
    /// \param Bd (input, optional) only copy these body data fields
    /// \param F  (input, optional) only copy bodies matching this flag
    snapshot(double       t,
	     bodies const&B,
	     fieldset     Bd=fieldset::all,
	     int          F =0) falcON_THROWING
    : bodies ( B,Bd,F ),
      INIT   ( true ),
      TINI   ( t ),
      TIME   ( t ) {}
    //--------------------------------------------------------------------------
    /// copy constructor from snapshot
    ///
    /// make a partial copy, only copying data specified by 3rd arg
    ///
    /// \param S  (input) set of bodies
    /// \param Bd (input, optional) only copy these body data fields
    /// \param F  (input, optional) only copy bodies matching this flag
    explicit
    snapshot(snapshot const&S,
	     fieldset       Bd=fieldset::all,
	     int            F =0 ) falcON_THROWING
    : bodies ( S,Bd,F ),
      INIT   ( S.INIT ),
      TINI   ( S.TINI ),
      TIME   ( S.TIME ) {}
    //--------------------------------------------------------------------------
    /// Copy: replace our data (if any) with those of other bodies (\b not 
    /// \b yet \b implemented)
    ///
    /// We only copy data specified by 2nd argument and copy only those bodies
    /// whose flag matches 3rd argument. Equivalent to (but more efficient than)
    /// destruction followed by constructor 2.
    ///
    /// \note \b NOT \b YET \b IMPLEMENTED
    ///
    /// \param S  (input) snapshot to be copied
    /// \param Bd (input, optional) body data to be copied
    /// \param f  (input, optional) flag for bodies to be copied
    void copy(snapshot const&S,
	      fieldset       Bd=fieldset::all,
	      int            f=0) falcON_THROWING 
    {
      bodies::copy(S,Bd,f);
      TIME = S.TIME;
    }
    //@}
    //==========================================================================
#ifdef falcON_NEMO
    //==========================================================================
    //                                                                          
    /// \name NEMO data I/O                                                     
    //@{                                                                        
    //                                                                          
    //==========================================================================
    /// Read a single NEMO snapshot if time in input is in desired time range;
    /// on return this mirrors the snapshot on disk (if it has been read)
    ///
    /// If time in input is in time range (4th arg), we read all data indicated
    /// by 3rd argument for all bodies in input (re-allocating if necessary).
    /// Previous data are lost.
    ///
    /// If the initial time was previously unset, we set it now.
    ///
    /// \return       true if time in input was in desired time range
    /// \param Is     (input) NEMO input stream containing snapshot
    /// \param Read   (output) body data fields which have been read
    /// \param Get    (input) body data fields to be read
    /// \param range  (input, optional) time range in NEMO range format
    ///               (default: read next snapshot in stream)
    /// \param warn   (input, optional) issue falcON::warning()s about missing
    ///               data? (default: issue warning)
    bool read_nemo(nemo_in const&Is,
		   fieldset     &Read,
		   fieldset      Get,
		   const char   *range=0,
		   bool          warn=1) falcON_THROWING;
    //--------------------------------------------------------------------------
    /// Read a part of a NEMO snapshot into a given body range
    ///
    /// Here, we read data for \e Nread (4th argument) bodies from the snapshot
    /// input (1st arg) into the bodies starting at \e start (3rd argument).
    /// Enough bodies must be have been allocated previously.
    ///
    /// If the initial time was previously unset, we set it now.
    ///
    /// \return      body data fields that have been read
    /// \param Si    (input) snapshot input
    /// \param Bd    (input) body data fields to be read
    /// \param start (input) first body to be read into
    /// \param Nread (input, optional) read this many (default: all in input)
    /// \param warn  (input, optional) issue falcON::warning()s about missing
    ///               data? (default: issue warning)
    fieldset read_nemo(snap_in const &Si,
		       fieldset       Bd,
		       iterator const&start,
		       unsigned       Nread=0,
		       bool           warn=1) falcON_THROWING;
    //--------------------------------------------------------------------------
    /// Generate a NEMO snapshot on disk from all bodies
    ///
    /// \param  Os  (input) nemo output stream
    /// \param  Bd  (input) body data fields to be written
    void write_nemo(nemo_out const&Os,
		    fieldset       Bd) const falcON_THROWING;
    //--------------------------------------------------------------------------
    /// Generate a NEMO snapshot on disk from a subset of all bodies
    ///
    /// We write body data fields specified by \e Bd (2nd argument) for \e 
    /// Nwrite (4th argument) bodies starting at \e start (3rd argument) in
    /// NEMO snapshot format.
    ///
    /// \param Os     (input) nemo output stream
    /// \param Bd     (input) body data fields to be written
    /// \param start  (input) first body to write
    /// \param Nwrite (input, optional) write this many (default: all)
    void write_nemo(nemo_out const&Os,
		    fieldset       Bd,
		    iterator const&start,
		    unsigned       Nwrite=0) const falcON_THROWING;
    //@}
    //--------------------------------------------------------------------------
#endif
  }; // class snapshot
  //////////////////////////////////////////////////////////////////////////////
} // namespace falcON {
// /////////////////////////////////////////////////////////////////////////////
///                                                                             
/// \name macros for looping over bodies                                        
//@{                                                                            
////////////////////////////////////////////////////////////////////////////////
#ifndef LoopAllBodies           /* loop all bodies                           */
/// This macro provides an easy way to loop over all bodies.
///
/// A typical usage would look like this \code
///   snapshot *S;
///   LoopAllBodies(S,b) {
///     b.pos() += dt*vel(b);
///     b.vel() += dt*acc(b);
///   } \endcode
///
/// \param PTER  valid pointer to falcON::bodies (or falcON::snapshot)
/// \param NAME  name given to loop variable (of type falcON::body)
#define LoopAllBodies(PTER,NAME)			\
  for(falcON::body NAME=(PTER)->begin_all_bodies();	\
                   NAME; ++NAME)
#endif
//------------------------------------------------------------------------------
#ifndef LoopTypedBodies         /* loop all bodies of a given type           */
/// This macro provides an easy way to loop over all bodies of a given 
/// falcON::bodytype
///
/// A typical usage would look like this \code
///   snapshot *S;
///   bodytype  t;
///   LoopTypedBodies(S,b,t) {
///     b.pos() += dt*vel(b);
///     b.vel() += dt*acc(b);
///   } \endcode
///
/// \param PTER  valid pointer to falcON::bodies (or falcON::snapshot)
/// \param NAME  name given to loop variable (of type falcON::body)
/// \param TYPE  the bodytype of the bodies to be looped over
#define LoopTypedBodies(PTER,NAME,TYPE)					\
  for(falcON::body NAME= (PTER)->begin_typed_bodies(TYPE);		\
                   NAME  != (PTER)->end_typed_bodies(TYPE); ++NAME)
#endif
//------------------------------------------------------------------------------
#ifndef LoopSPHBodies           /* loop all SPH bodies                       */
/// This macro provides an easy way to loop over all SPH bodies
///
/// A typical usage would look like this \code
///   snapshot *S;
///   LoopSPHBodies(S,b,t) {
///     b.pos() += dt*vel(b);
///     b.vel() += dt*acc(b);
///     b.uin() += dt*udin(b);
///   } \endcode
///
/// \param PTER  valid pointer to falcON::bodies (or falcON::snapshot)
/// \param NAME  name given to loop variable (of type falcON::body)
#define LoopSPHBodies(PTER,NAME)		\
  LoopTypedBodies(PTER,NAME,bodytype::gas)
#endif
//------------------------------------------------------------------------------
#ifndef LoopSTDBodies           /* loop all non-SPH bodies                   */
/// This macro provides an easy way to loop over all non-SPH bodies
///
/// A typical usage would look like this \code
///   snapshot *S;
///   LoopSTDBodies(S,b,t) {
///     b.pos() += dt*vel(b);
///     b.vel() += dt*acc(b);
///   } \endcode
///
/// \param PTER  valid pointer to falcON::bodies (or falcON::snapshot)
/// \param NAME  name given to loop variable (of type falcON::body)
#define LoopSTDBodies(PTER,NAME)		\
  LoopTypedBodies(PTER,NAME,bodytype::std)
#endif
//------------------------------------------------------------------------------
#ifndef LoopBodyPairs           /* loop all bodies after a given body        */
/// This macro provides an easy way to loop over all body pairs
///
/// A typical usage would look like this \code
///   snapshot *S;
///   LoopAllBodies(S,bi)
///     LoopBodyPairs(bi,bj) {
///       real Rij_squared = dist_sq(pos(bi),pos(bj));
///     } \endcode
/// \param NAME1  name of the first body in an outer loop
/// \param NAME2  name given to inner-loop variable (of type falcON::body)
#define LoopBodyPairs(NAME1,NAME2)		\
  for(falcON::body NAME2(NAME1,1); NAME2; ++NAME2)
#endif
//------------------------------------------------------------------------------
//@}
////////////////////////////////////////////////////////////////////////////////
#endif // falcON_included_body_h
