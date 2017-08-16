#include <fstream>
#include <sstream>
#include <numeric>
#include <algorithm>

#include "Tools/Exception/exception.hpp"
#include "Tools/Code/LDPC/AList/AList.hpp"

#include "Codec_LDPC.hpp"

using namespace aff3ct;
using namespace aff3ct::tools;

template <typename B, typename Q>
Codec_LDPC<B,Q>
::Codec_LDPC(const factory::Encoder_LDPC::parameters &enc_params,
             const factory::Decoder_LDPC::parameters &dec_params,
             const int n_threads)
: Codec_SISO<B,Q>(enc_params, dec_params), enc_par(enc_params), dec_par(dec_params),
  info_bits_pos(enc_params.K),
  decoder_siso (n_threads, nullptr)
{
	bool is_info_bits_pos = false;
	if (!enc_par.G_alist_path.empty() && enc_params.type == "LDPC")
	{
		std::ifstream file_G(enc_par.G_alist_path, std::ifstream::in);
		G = AList::read(file_G);

		try
		{
			info_bits_pos = AList::read_info_bits_pos(file_G, enc_params.K, enc_params.N_cw);
			is_info_bits_pos = true;
		}
		catch (std::exception const&)
		{
			// information bits positions are not in the G matrix file
		}

		file_G.close();
	}

	std::ifstream file_H(dec_params.H_alist_path, std::ifstream::in);
	H = AList::read(file_H);

	if (!is_info_bits_pos)
	{
		try
		{
			if (enc_params.type == "LDPC_H")
			{
				auto encoder_LDPC = this->build_encoder();
				encoder_LDPC->get_info_bits_pos(info_bits_pos);
				delete encoder_LDPC;
			}
			else
				info_bits_pos = AList::read_info_bits_pos(file_H, enc_params.K, enc_params.N_cw);
		}
		catch (std::exception const&)
		{
			std::iota(info_bits_pos.begin(), info_bits_pos.end(), 0);
		}
	}

	file_H.close();
}

template <typename B, typename Q>
Codec_LDPC<B,Q>
::~Codec_LDPC()
{
}

template <typename B, typename Q>
module::Encoder_LDPC<B>* Codec_LDPC<B,Q>
::build_encoder(const int tid, const module::Interleaver<B>* itl)
{
	return factory::Encoder_LDPC::build<B>(enc_par, G, H);
}

template <typename B, typename Q>
module::Decoder_SISO_SIHO<B,Q>* Codec_LDPC<B,Q>
::_build_siso(const int tid, const module::Interleaver<Q>* itl, module::CRC<B>* crc)
{
	decoder_siso[tid] = factory::Decoder_LDPC::build_siso<B,Q>(dec_par, H, info_bits_pos);
	return decoder_siso[tid];
}

template <typename B, typename Q>
module::Decoder_SISO<Q>* Codec_LDPC<B, Q>
::build_siso(const int tid, const module::Interleaver<Q>* itl, module::CRC<B>* crc)
{
	return this->_build_siso(tid, itl, crc);
}

template <typename B, typename Q>
module::Decoder_SIHO<B,Q>* Codec_LDPC<B,Q>
::build_decoder(const int tid, const module::Interleaver<Q>* itl, module::CRC<B>* crc)
{
	if (decoder_siso[tid] != nullptr)
	{
		auto ptr = decoder_siso[tid];
		decoder_siso[tid] = nullptr;
		return ptr;
	}
	else
	{
		auto decoder = factory::Decoder_LDPC::build<B,Q>(dec_par, H, info_bits_pos);
		decoder_siso[tid] = nullptr;
		return decoder;
	}
}

template <typename B, typename Q>
void Codec_LDPC<B,Q>
::extract_sys_par(const mipp::vector<Q> &Y_N, mipp::vector<Q> &sys, mipp::vector<Q> &par)
{
	const auto K = this->enc_params.K;
	const auto N = this->enc_params.N_cw;

	if ((int)Y_N.size() != N * this->enc_params.n_frames)
	{
		std::stringstream message;
		message << "'Y_N.size()' has to be equal to 'N' * 'inter_frame_level' ('Y_N.size()' = " << Y_N.size()
		        << ", 'N' = " << N << ", 'inter_frame_level' = " << this->enc_params.n_frames << ").";
		throw length_error(__FILE__, __LINE__, __func__, message.str());
	}

	if ((int)sys.size() != K * this->enc_params.n_frames)
	{
		std::stringstream message;
		message << "'sys.size()' has to be equal to 'K' * 'inter_frame_level' ('sys.size()' = " << sys.size()
		        << ", 'K' = " << K << ", 'inter_frame_level' = " << this->enc_params.n_frames << ").";
		throw length_error(__FILE__, __LINE__, __func__, message.str());
	}

	if ((int)par.size() != (N - K) * this->enc_params.n_frames)
	{
		std::stringstream message;
		message << "'par.size()' has to be equal to ('N' - 'K') * 'inter_frame_level' ('par.size()' = " << par.size()
		        << ", 'N' = " << N << ", 'K' = " << K << ", 'inter_frame_level' = "
		        << this->enc_params.n_frames << ").";
		throw length_error(__FILE__, __LINE__, __func__, message.str());
	}

	// extract systematic and parity information
	auto sys_idx = 0;
	for (auto f = 0; f < this->enc_params.n_frames; f++)
	{
		for (auto i = 0; i < K; i++)
			sys[f * K +i] = Y_N[f * N + info_bits_pos[i]];

		for (auto i = 0; i < N; i++)
			if (std::find(info_bits_pos.begin(), info_bits_pos.end(), i) != info_bits_pos.end())
			{
				par[sys_idx] = Y_N[f * N +i];
				sys_idx++;
			}
	}
}

// ==================================================================================== explicit template instantiation 
#include "Tools/types.h"
#ifdef MULTI_PREC
template class aff3ct::tools::Codec_LDPC<B_8,Q_8>;
template class aff3ct::tools::Codec_LDPC<B_16,Q_16>;
template class aff3ct::tools::Codec_LDPC<B_32,Q_32>;
template class aff3ct::tools::Codec_LDPC<B_64,Q_64>;
#else
template class aff3ct::tools::Codec_LDPC<B,Q>;
#endif
// ==================================================================================== explicit template instantiation
