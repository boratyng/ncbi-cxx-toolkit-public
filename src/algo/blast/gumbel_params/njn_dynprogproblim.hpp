#ifndef ALGO_BLAST_GUMBEL_PARAMS__INCLUDED_NJN_DYNPROGPROBLIM
#define ALGO_BLAST_GUMBEL_PARAMS__INCLUDED_NJN_DYNPROGPROBLIM

/* $Id$
* ===========================================================================
*
*                            PUBLIC DOMAIN NOTICE
*               National Center for Biotechnology Information
*
*  This software/database is a "United States Government Work" under the
*  terms of the United States Copyright Act.  It was written as part of
*  the author's offical duties as a United States Government employee and
*  thus cannot be copyrighted.  This software/database is freely available
*  to the public for use. The National Library of Medicine and the U.S.
*  Government have not placed any restriction on its use or reproduction.
*
*  Although all reasonable efforts have been taken to ensure the accuracy
*  and reliability of the software and data, the NLM and the U.S.
*  Government do not and cannot warrant the performance or results that
*  may be obtained by using this software or data. The NLM and the U.S.
*  Government disclaim all warranties, express or implied, including
*  warranties of performance, merchantability or fitness for any particular
*  purpose.
*
*  Please cite the author in any work or product based on this material.
*
* ===========================================================================*/

/*****************************************************************************

File name: njn_dynprogproblim.hpp

Author: John Spouge

Contents: 

******************************************************************************/

#include <corelib/ncbitype.h>
#include <corelib/ncbi_limits.h>

#include "njn_dynprogprob.hpp"
#include "sls_alp_data.hpp"

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(blast)

BEGIN_SCOPE(Njn)


    class DynProgProbLim : public DynProgProb {

        // DynProgProbLim performs updates for probabilities in a dynamic programming computation.
        //    The object limits storage to [valueLower_, valueUpper_).
        //    Probability outside the limits can be obtained from getProbLost ().
        //
        // The object behaves as follows:
        //
        // Default:
        //    (1) The initial state of the dynamic programming computation is 0 with probability 1.0.
        //
        // Behavior:
        //    (2) If input_ is the computation's input, it replaces oldValue_ with ValueFct (oldValue_, input_)
        //    (3) The dynamic programming function can be reset with setValueFct (...).
        //    (4) The probability for the input states can be reset with setInput (...).
        //    (5) The probability of input_ = [0, dimInputProb_) is inputProb_ [input_].
        //    (6) getProb (Int4 i_) returns the probability corresponding to the Int4 value i_.
        //    (7) The values are limited to [d_valueBegin, d_valueEnd), which are initialized with [valueLower_, valueUpper_).
        //    (8) getLostProb () returns the lost probability.

        public:

        inline DynProgProbLim ( 
        ValueFct *valueFct_ = 0, // function for updating dynamic programming values
        size_t dimInputProb_ = 0, 
        const double *inputProb_ = 0, // array of input states : d_inputProb_p [0...dimInputProb - 1]
        // The following behave like arguments to clear ().
        Int4 valueLower_ = 0, // lower Int4 value corresponding to the "probability" array
        Int4 valueUpper_ = 0, // one beyond present upper Int4 value corresponding to the "probability" array
        const double *prob_ = 0) // "probabilities" prob [valueLower_, valueUpper_) corresponding to the Int4s
        // default prob_ == 0 : equivalent to
        //    prob_ [0] = 1.0
        //    assert (valueLower_ <= 0);
        //    assert (0 < valueUpper_);
            : DynProgProb (valueFct_, dimInputProb_, inputProb_, valueLower_, valueUpper_, prob_), 
            d_probLost (0.0)
        {}

        inline DynProgProbLim (const DynProgProbLim &dynProgProbLim_)
            : DynProgProb (dynProgProbLim_), 
            d_probLost (dynProgProbLim_.getProbLost ())
        {}

        virtual inline ~DynProgProbLim () {}

        virtual inline DynProgProbLim &operator= (const DynProgProbLim &dynProgProbLim_)
        {
            if (this != &dynProgProbLim_) copy (dynProgProbLim_);
            return *this;
        }

        // added to eliminate compiler warning
        virtual inline DynProgProbLim &operator= (const DynProgProb &dynProgProb_)
        {
            throw Sls::error("Assignment operator not implemented", 4);
            return *this;
        }

        virtual inline void copy (const DynProgProbLim &dynProgProbLim_)
        {
            copy (dynProgProbLim_, dynProgProbLim_.getProbLost ());
        }

        virtual inline void copy (
            const DynProgProb &dynProgProb_, // base object
        double probLost_) // lower limit for Int4 values in the array (an offset)
        {
            DynProgProb::copy (dynProgProb_);
            d_probLost = probLost_;
        }

        // added to eliminate compiler warning
        virtual inline void copy (const DynProgProb &dynProgProb_)
        {
            throw Sls::error("Virtual function not implemented", 4);
        }

        // added to eliminate compiler warning
        virtual void copy (
            size_t step_, // current index : starts at 0 
            const double *const *array_, // two corresponding arrays of probabilities 
        size_t arrayCapacity_, // present capacity of the array
        Int4 valueBegin_ = 0, // lower limit for Int4 values in the array (an offset)
        Int4 valueLower_ = 0, // present lower Int4 value in the array
        Int4 valueUpper_ = 0, // one beyond present upper Int4 value in the array
        ValueFct *valueFct_ = 0, // function for updating dynamic programming values
        size_t dimInputProb_ = 0, 
        const double *inputProb_ = 0) // array of input states : d_inputProb_p [0...dimInputProb - 1]
        {
            throw Sls::error("Virtual function not implemented", 4);
        }


        virtual void setLimits ( 
        Int4 valueBegin_ = 0, // Range for values is [valueBegin_, valueEnd_).
        Int4 valueEnd_ = 0); // Range for values is [valueBegin_, valueEnd_).
        // resets the limits, transferring any probability outside the new limit to d_probLost

        virtual void update (); // updates dynamic prog probs 
        // assert (getValueFct ());
        // assert (getDimInputProb ());
        // assert (getInputProb ());

        virtual inline void clear ( // restarts the computation
        Int4 valueLower_, // lower Int4 value corresponding to the "probability" array
        Int4 valueUpper_ = 0, // one beyond present upper Int4 value corresponding to the "probability" array
        const double *prob_ = 0) // "probabilities" prob_ [valueLower_, valueUpper_) corresponding to the Int4s
        // default prob_ = 0 corresponds to probability 1.0 at value = 0
        // assumes prob_ [valueLower_, valueUpper_) 
        {
            DynProgProb::clear (valueLower_, valueUpper_, prob_);
            d_probLost = 0.0;
        }

        // added to eliminate icc compiler warning about partial overriding
        virtual void clear (
        Int4 valueBegin_, // lower limit for Int4 values in the array (an offset)
        size_t arrayCapacity_)  // new array capacity 
        {
            DynProgProb::clear (valueBegin_, arrayCapacity_);
        }

        virtual inline void clear () {clear (0);}

        using DynProgProb::operator bool; // ? is the object ready for computation ?
        using DynProgProb::setValueFct;
        using DynProgProb::setInput;
        using DynProgProb::getProb; // probability value
        using DynProgProb::getStep; // current index : starts at 0 
        using DynProgProb::getArray; // two corresponding arrays of probabilities d_array_p [0,1][0...d_arrayCapacity - 1]
        using DynProgProb::getArrayCapacity; // # (different values)
        using DynProgProb::getValueBegin; // lower limit for Int4 values in the array (an offset)
        using DynProgProb::getValueLower; // present lower Int4 value in the array
        using DynProgProb::getValueUpper; // one beyond present upper Int4 value in the array
        using DynProgProb::getValueFct; // function for updating dynamic programming values
        using DynProgProb::getDimInputProb;
        using DynProgProb::getInputProb; // array of input states : d_inputProb_p [0...dimInputProb - 1]

        virtual inline double getProbLost () const {return d_probLost;} // probability outside limits

        private:

            double d_probLost; // probability outside limits

        virtual void reserve (size_t arrayCapacity_); // new array capacity
        // reduces capacity of and copies d_array_p from its begin
        
        virtual void setValueBegin (Int4 valueBegin_); // lowest possible Int4 value in the array

        using DynProgProb::getValueEnd; // one beyond largest possible Int4 value in present range
        using DynProgProb::getArrayPos; // offset for array position containing value_
   };

END_SCOPE(Njn)

END_SCOPE(blast)
END_NCBI_SCOPE

#endif //! ALGO_BLAST_GUMBEL_PARAMS__INCLUDED_NJN_DYNPROGPROBLIM