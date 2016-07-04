#ifndef DECODER_RSC_BCJR_SEQ_GENERIC_HPP_
#define DECODER_RSC_BCJR_SEQ_GENERIC_HPP_

#include <vector>
#include "../../../../Tools/MIPP/mipp.h"

#include "../Decoder_RSC_BCJR.hpp"

template <typename B, typename R>
class Decoder_RSC_BCJR_seq_generic : public Decoder_RSC_BCJR<B,R>
{
protected:
	mipp::vector<mipp::vector<R>> alpha; // node metric (left to right)
	mipp::vector<mipp::vector<R>> beta;  // node metric (right to left)
	mipp::vector<mipp::vector<R>> gamma; // edge metric

public:
	Decoder_RSC_BCJR_seq_generic(const int &K, const std::vector<std::vector<int>> &trellis, const bool buffered_encoding = true);
	virtual ~Decoder_RSC_BCJR_seq_generic();

	virtual void decode(const mipp::vector<R> &sys, const mipp::vector<R> &par, mipp::vector<R> &ext) = 0;
};

#include "Decoder_RSC_BCJR_seq_generic.hxx"

#endif /* DECODER_RSC_BCJR_SEQ_GENERIC_HPP_ */
