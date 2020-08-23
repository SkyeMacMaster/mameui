// license:GPL-2.0+
// copyright-holders:Couriersud

#include "palloc.h"
#include "pstonum.h"
#include "ptokenizer.h"
#include "putil.h"

namespace plib {

	PERRMSGV(MF_EXPECTED_TOKEN_1_GOT_2,         2, "Expected token <{1}>, got <{2}>")
	PERRMSGV(MF_EXPECTED_STRING_GOT_1,          1, "Expected a string, got <{1}>")
	PERRMSGV(MF_EXPECTED_IDENTIFIER_GOT_1,      1, "Expected an identifier, got <{1}>")
	PERRMSGV(MF_EXPECTED_ID_OR_NUM_GOT_1,       1, "Expected an identifier or number, got <{1}>")
	PERRMSGV(MF_EXPECTED_NUMBER_GOT_1,          1, "Expected a number, got <{1}>")
	PERRMSGV(MF_EXPECTED_LONGINT_GOT_1,         1, "Expected a logn int, got <{1}>")
	PERRMSGV(MF_EXPECTED_LINENUM_GOT_1,         1, "Expected line number after line marker but got <{1}>")
	PERRMSGV(MF_EXPECTED_FILENAME_GOT_1,        1, "Expected file name after line marker but got <{1}>")

	// ----------------------------------------------------------------------------------------
	// A simple tokenizer
	// ----------------------------------------------------------------------------------------

	void ptokenizer::skipeol()
	{
		pstring::value_type c = getc();
		while (c != 0)
		{
			if (c == 10)
			{
				c = getc();
				if (c != 13)
					ungetc(c);
				return;
			}
			c = getc();
		}
	}

	pstring::value_type ptokenizer::getc()
	{
		if (m_unget != 0)
		{
			pstring::value_type c = m_unget;
			m_unget = 0;
			return c;
		}
		if (m_px == m_cur_line.end())
		{
			//++m_source_location.back();
			if (m_strm->readline_lf(m_cur_line))
			{
				m_px = m_cur_line.begin();
				if (*m_px != '#')
					m_token_queue->push_back(token_t(token_type::SOURCELINE, m_cur_line));
			}
			else
				return 0;
		}
		pstring::value_type c = *(m_px++);
		return c;
	}

	void ptokenizer::ungetc(pstring::value_type c)
	{
		m_unget = c;
	}

	void ptoken_reader::require_token(const token_id_t &token_num)
	{
		require_token(get_token(), token_num);
	}
	void ptoken_reader::require_token(const token_t &tok, const token_id_t &token_num)
	{
		if (!tok.is(token_num))
		{
			error(MF_EXPECTED_TOKEN_1_GOT_2(token_num.name(), tok.str()));
		}
	}

	pstring ptoken_reader::get_string()
	{
		token_t tok = get_token();
		if (!tok.is_type(token_type::STRING))
		{
			error(MF_EXPECTED_STRING_GOT_1(tok.str()));
		}
		return tok.str();
	}


	pstring ptoken_reader::get_identifier()
	{
		token_t tok = get_token();
		if (!tok.is_type(token_type::IDENTIFIER))
		{
			error(MF_EXPECTED_IDENTIFIER_GOT_1(tok.str()));
		}
		return tok.str();
	}

	pstring ptoken_reader::get_identifier_or_number()
	{
		token_t tok = get_token();
		if (!(tok.is_type(token_type::IDENTIFIER) || tok.is_type(token_type::NUMBER)))
		{
			error(MF_EXPECTED_ID_OR_NUM_GOT_1(tok.str()));
		}
		return tok.str();
	}

	// FIXME: combine into template
	double ptoken_reader::get_number_double()
	{
		token_t tok = get_token();
		if (!tok.is_type(token_type::NUMBER))
		{
			error(MF_EXPECTED_NUMBER_GOT_1(tok.str()));
		}
		bool err(false);
		auto ret = plib::pstonum_ne<double>(tok.str(), err);
		if (err)
			error(MF_EXPECTED_NUMBER_GOT_1(tok.str()));
		return ret;
	}

	long ptoken_reader::get_number_long()
	{
		token_t tok = get_token();
		if (!tok.is_type(token_type::NUMBER))
		{
			error(MF_EXPECTED_LONGINT_GOT_1(tok.str()) );
		}
		bool err(false);
		auto ret = plib::pstonum_ne<long>(tok.str(), err);
		if (err)
			error(MF_EXPECTED_LONGINT_GOT_1(tok.str()) );
		return ret;
	}

	ptoken_reader::token_t ptoken_reader::get_token()
	{
		token_t ret = get_token_queue();
		while (true)
		{
			if (ret.is_type(token_type::token_type::ENDOFFILE))
				return ret;

			//printf("%s\n", ret.str().c_str());
			if (ret.is_type(token_type::LINEMARKER))
			{
				bool benter(false);
				bool bexit(false);
				pstring file;
				unsigned lineno(0);

				ret = get_token_queue();
				if (!ret.is_type(token_type::NUMBER))
					error(MF_EXPECTED_LINENUM_GOT_1(ret.str()));
				lineno = pstonum<unsigned>(ret.str());
				ret = get_token_queue();
				if (!ret.is_type(token_type::STRING))
					error(MF_EXPECTED_FILENAME_GOT_1(ret.str()));
				file = ret.str();
				ret = get_token_queue();
				while (ret.is_type(token_type::NUMBER))
				{
					if (ret.str() == "1")
						benter = true;
					if (ret.str() == "2")
						bexit = true;
					// FIXME: process flags; actually only 1 (file enter) and 2 (after file exit)
					ret = get_token_queue();
				}
				if (bexit) // pop the last location
					m_source_location.pop_back();
				if (!benter) // new location!
					m_source_location.pop_back();
				m_source_location.emplace_back(plib::source_location(file, lineno - 1));
			}
			else if (ret.is_type(token_type::SOURCELINE))
			{
				m_line = ret.str();
				++m_source_location.back();
				ret = get_token_queue();
			}
			else
			{
				return ret;
			}
		}
	}

	ptoken_reader::token_t ptokenizer::get_token_internal()
	{
		// skip ws
		pstring::value_type c = getc();
		while (m_whitespace.find(c) != pstring::npos)
		{
			c = getc();
			if (eof())
			{
				return token_t(token_type::ENDOFFILE);
			}
		}
		if (m_support_line_markers && c == '#')
			return token_t(token_type::LINEMARKER, "#");

		if (m_number_chars_start.find(c) != pstring::npos)
		{
			// read number while we receive number or identifier chars
			// treat it as an identifier when there are identifier chars in it
			token_type ret = token_type::NUMBER;
			pstring tokstr = "";
			while (true) {
				if (m_identifier_chars.find(c) != pstring::npos && m_number_chars.find(c) == pstring::npos)
					ret = token_type::IDENTIFIER;
				else if (m_number_chars.find(c) == pstring::npos)
					break;
				tokstr += c;
				c = getc();
			}
			ungetc(c);
			return token_t(ret, tokstr);
		}

		// not a number, try identifier
		if (m_identifier_chars.find(c) != pstring::npos)
		{
			// read identifier till non identifier char
			pstring tokstr = "";
			while (m_identifier_chars.find(c) != pstring::npos)
			{
				tokstr += c;
				c = getc();
			}
			ungetc(c);
			auto id = m_tokens.find(tokstr);
			return (id != m_tokens.end()) ?
					token_t(id->second, tokstr)
				:   token_t(token_type::IDENTIFIER, tokstr);
		}

		if (c == m_string)
		{
			pstring tokstr = "";
			c = getc();
			while (c != m_string)
			{
				tokstr += c;
				c = getc();
			}
			return token_t(token_type::STRING, tokstr);
		}
		else
		{
			// read identifier till first identifier char or ws
			pstring tokstr = "";
			while ((m_identifier_chars.find(c) == pstring::npos) && (m_whitespace.find(c) == pstring::npos))
			{
				tokstr += c;
				// expensive, check for single char tokens
				if (tokstr.length() == 1)
				{
					auto id = m_tokens.find(tokstr);
					if (id != m_tokens.end())
						return token_t(id->second, tokstr);
				}
				c = getc();
			}
			ungetc(c);
			auto id = m_tokens.find(tokstr);
			return (id != m_tokens.end()) ?
					token_t(id->second, tokstr)
				:   token_t(token_type::UNKNOWN, tokstr);
		}
	}

	ptoken_reader::token_t ptokenizer::get_token_comment()
	{
		token_t ret = get_token_internal();
		while (true)
		{
			if (ret.is_type(token_type::token_type::ENDOFFILE))
				return ret;

			if (ret.is(m_tok_comment_start))
			{
				do {
					ret = get_token_internal();
				} while (ret.is_not(m_tok_comment_end));
				ret = get_token_internal();
			}
			else if (ret.is(m_tok_line_comment))
			{
				skipeol();
				ret = get_token_internal();
			}
			else
			{
				return ret;
			}
		}
	}


	void ptoken_reader::error(const perrmsg &errs)
	{
		pstring s("");
		pstring trail      ("                 from ");
		pstring trail_first("In file included from ");
		pstring e = plib::pfmt("{1}:{2}:0: error: {3}\n")
				(m_source_location.back().file_name(), m_source_location.back().line(), errs());
		m_source_location.pop_back();
		while (!m_source_location.empty())
		{
			if (m_source_location.size() == 1)
				trail = trail_first;
			s = plib::pfmt("{1}{2}:{3}:0\n{4}")(trail, m_source_location.back().file_name(), m_source_location.back().line(), s);
			m_source_location.pop_back();
		}
		verror("\n" + s + e + " " + m_line + "\n");
	}

} // namespace plib
