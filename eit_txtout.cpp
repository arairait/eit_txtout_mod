// TsEITRead.cpp : �R���\�[�� �A�v���P�[�V�����̃G���g�� �|�C���g���`���܂��B
//
#include "stdafx.h"
#include "tsdata.h"
#include "ts_sidef.h"
#include "ts_packet.h"
#include "wingetopt.h"

//TODO: -C (range)	�y����z(output)���e�L�X�g�t�@�C���ł͂Ȃ��A����TS�t�@�C���̎w�肵���͈͂�؂��������̂ɂȂ�܂�
//			(range)��(start_packet_idx):(end_packet_idx)
using namespace std;

#define			STR_STL_LOCALE	"japanese"					// STL��locale�ɓn��������
#define			MAX_PID_NUM		8192

//#define _TEST_USE_FS_

// main�̕Ԃ�l
enum
{
	ERR_PARAM = 1,
	ERR_INPUTFILE,
	ERR_OUTPUTFILE,
	ERR_NOTOT,
	ERR_NOPAT,
	ERR_NOSDT,
};

// PID���̃Z�N�V�����o�b�t�@
// map���g���Ă��������ߖ�ʂ͒m��Ă�̂Ŕz��Ŏ���
static CSectionBuf	*ev_pSectionBuf[MAX_PID_NUM];

static UINT64		ev_BefSectionFilePos[MAX_PID_NUM];		// �ȑO�̃t�@�C���ʒu(�擪:1)�Bpacket�P��(byte�P�ʂł͂Ȃ�)

static UINT64		ev_cur_filepos;							// ���݂̃t�@�C���ʒu(�擪:1)�Bpacket�P��(byte�P�ʂł͂Ȃ�)

// �I�v�V�����p�����[�^
enum E_READ_SIZE{RS_SIZE,RS_RATIO,RS_SEC,};
static bool			ev_opt_is_out_patpmt = false;			// PAT/PMT���o�͂���H
static bool			ev_opt_is_all_out = false;				// �S�T�[�r�X/�C�x���g�����o�͂���H
static bool			ev_opt_is_out_extevt = false;			// �g���`���C�x���g�L�q�q���o�͂���H
static int			ev_opt_offset_read = 0;					// �ǂݍ��݃I�t�Z�b�g
static bool			ev_opt_offset_read_is_ratio = false;
static int			ev_opt_max_read_size = 0;				// �ő�ǂݍ��݃T�C�Y(*188*1000byte)
static E_READ_SIZE	ev_opt_max_read_type = RS_SIZE;
static int			ev_opt_offset_beg = 0;					// �J�n�I�t�Z�b�g�b��
static int			ev_opt_offset_end = 0;					// �I���I�t�Z�b�g�b��
static bool			ev_opt_read_eit_sch = false;			// EIT(�X�P�W���[��)��ǂݍ��ށH
static bool			ev_opt_verbose = false;					// �ڍ׏o�̓��[�h
static bool			ev_opt_no_progress = false;				// �i���\�����o���Ȃ�
static bool			ev_opt_outtime_offset = false;			// 
static int			ev_opt_evt_output_type = 0;
static const TCHAR	*ev_opt_input_fname = NULL;				// ���ݏ������Ă�TS�t�@�C����(�f�o�b�O���O�p��)
static const TCHAR	*ev_opt_output_fname = NULL;			// �o�̓t�@�C��
static const TCHAR	*ev_opt_dbgout_fname = NULL;
static const TCHAR	*ev_opt_dbglog_fname = NULL;

// 
static bool			ev_dbglog_flg = false;					// �f�o�b�O���O�t���O�B�܂�1����o�͂��ĂȂ���FALSE
static tostream		*ev_ps_dbglog = NULL;					// �f�o�b�O���O�o�͗p�X�g���[��


// TS�t�@�C�����̔ԑg���o�b�t�@�����O�p
static vector<CPAT>		ev_pats;
static vector<CEIT*>	ev_peits;		// �������񎝂\��������Ainsert�Ƃ��̎��s�R�X�g���o�J�ɂȂ�Ȃ���ŁA�|�C���^�Ŏ���
static vector<CSDT*>	ev_psdts;		// ����
static vector<CPMT>		ev_pmts;
static vector<CTOT>		ev_tots;		// 2�����ێ�����

// eit1 < eit2�̎� true��Ԃ�
static bool _compare_eit(const CEIT *eit1, const CEIT *eit2)
{
	const CServiceInfo *p = eit2;
	return eit1->CompareTo( p, true ) > 0;
}

// pmt1 < pmt2�̎� true��Ԃ�
static bool _compare_pmt(const CPMT &pmt1, const CPMT &pmt2)
{
	const CServiceInfo *p = &pmt2;
	return pmt1.CompareTo( p, true ) > 0;
}

// sdt1 < sdt2�̎� true��Ԃ�
static bool _compare_sdt(const CSDT *sdt1, const CSDT *sdt2)
{
	const CServiceInfo *p = sdt2;
	return sdt1->CompareTo( p, true ) > 0;
}

// �Ԃ�l�F�ȉ��̂����ꂩ�BCRC�G���[�Ƃ��������ꍇ�͌㑱�p�P�b�g������͂�
enum eSECTION_RET
{
	SEC_NG_NEXT = 0,			// NG�B���p�P�b�g�v��
	SEC_OK_END,					// OK�B�Z�N�V�����f�[�^�I��
	SEC_OK_RETRY,				// OK�B
};
static eSECTION_RET _dump_section_data(const CPacket &packet, CSectionBuf *p_sec_buf, bool is_read_eit_sch)
{
	eSECTION_RET	ret = SEC_NG_NEXT;
	size_t			sec_len = 0;
	int				dbg_flg = 0;				// �f�o�b�O�p�t���O
	switch ( packet.header.pid )
	{
	case PID_PAT:
		// �ȈՓI�ɖ�����
		if ( ev_pats.size() == 0 
			|| ev_pats.back().version != CServiceInfo::GetVersion(p_sec_buf->GetData()) 
			|| ev_pats.back().id != CServiceInfo::GetID(p_sec_buf->GetData())
			)
		{
			CPAT	pat;
			if ( pat.SetData( p_sec_buf->GetData() ) )
			{
				ev_pats.push_back( pat );
				sec_len = pat.GetBodySize();
				ret = SEC_OK_END;
			}
		}
		else
			return SEC_OK_END;		// �d���f�[�^�Ȃ̂ŏ����ς݂ɂ��Ƃ�
		break;
	case PID_EIT_0:
	case PID_EIT_1:
	case PID_EIT_2:
		{
			//CEIT	eit;
			CEIT	*peit = new CEIT();
			bool	is_ignore;
			if ( peit->SetData( p_sec_buf->GetData(), !is_read_eit_sch, &is_ignore ) )
			{
//#ifdef _DEBUG
#if 0
				if ( peit->transport_stream_id == 16625 && peit->id == 101 && peit->table_id == 0x58 )
				{
					DBG_LOG_WARN( _T("version:%2d section:%3d segment_last:%3d last_section:%3d length:%4d\n"), 
						peit->version, peit->section, peit->segment_last_section_number, peit->last_section, peit->length );
				//	_CrtDbgBreak();
				}
#endif
				sec_len = peit->GetBodySize(); 
				if ( !is_ignore )
				{
					vector<CEIT*>::iterator pos = ev_peits.end();
					if ( ev_peits.size() > 0 )
						pos = lower_bound( ev_peits.begin(), ev_peits.end(), peit, _compare_eit );
					int idx = pos - ev_peits.begin();
					if ( pos == ev_peits.end() )
					{
						ev_peits.push_back( peit );
						dbg_flg = 1;
					}
					else if ( (*pos)->CompareTo( peit, true ) == 0 )
					{
						(*pos)->AddData( *peit );
						// �C�x���g�e�[�u���̂Ȃ����̂ɕt���Ă�EIT�e�[�u���ɂ͓���Ȃ�
						if ( (*pos)->IsFullProcSection() && (*pos)->event_tbl.size() == 0 )
						{
							delete ev_peits[idx];
							ev_peits.erase( pos );
						}
						dbg_flg = 2;
						delete peit;
					}
					else
					{
						ev_peits.insert( pos, peit );
						dbg_flg = 3;
					}
				}
				else
					delete peit;
				ret = SEC_OK_END;
			}
			else
			{
				ret = SEC_NG_NEXT;
				delete peit;
			}
		}
		break;
	case PID_TOT:
		{
			CTOT	tot;
			if ( tot.SetData( p_sec_buf->GetData() ) )
			{
				// �擪�����̑}���܂��͒ǉ�
				if ( ev_tots.size() == 0 )
					ev_tots.push_back( tot );
				else if ( ev_tots.size() > 1 && tot.jst_time < ev_tots[0].jst_time )
				{
					ev_tots.erase( ev_tots.begin() );
					ev_tots.insert( ev_tots.begin(), tot );
					_ASSERT( FALSE );			// ���n��ɕ���ł�͂��Ȃ�ŁA�ꉞ�x��
				}

				// ���������̒ǉ�
				if		( ev_tots.size() < 2 )
				{
					ev_tots.push_back( tot );
				}
				else if ( tot.jst_time > ev_tots[1].jst_time )
				{	
					ev_tots.pop_back();
					ev_tots.push_back( tot );
				}
				else
					_ASSERT( FALSE );

				sec_len = tot.GetBodySize(); 
				_ASSERT( ev_tots.size() <= 2 );
				
				ret = SEC_OK_END;
			}
		}
		break;
	case PID_SIT:
		//
		break;
	case PID_SDT:
		{
			CSDT	*psdt = new CSDT();
			if ( psdt->SetData( p_sec_buf->GetData() ) )
			{
				vector<CSDT*>::iterator pos = ev_psdts.end();
				sec_len = psdt->GetBodySize(); 
				if ( ev_psdts.size() > 0 )
					pos = lower_bound( ev_psdts.begin(), ev_psdts.end(), psdt, _compare_sdt );
				if ( pos == ev_psdts.end() )
				{
					ev_psdts.push_back( psdt );
				}
				else if ( (*pos)->CompareTo( psdt, true ) == 0 )
				{
					(*pos)->AddData( *psdt );
					delete psdt;
				}
				else
				{
					ev_psdts.insert( pos, psdt );
				}
				ret = SEC_OK_END;

			}
			else
			{
				ret = SEC_NG_NEXT;
				delete psdt;
			}
		}
		break;
	default:
		if ( ev_opt_is_out_patpmt )
		{
			CPMT	pmt;
			pmt.m_self_pid = packet.header.pid;
			pmt.m_file_pos = ev_cur_filepos;
			if ( pmt.SetData( p_sec_buf->GetData() ) )
			{
				vector<CPMT>::iterator pos = ev_pmts.end();
				if ( ev_pmts.size() > 0 )
					pos = lower_bound( ev_pmts.begin(), ev_pmts.end(), pmt, _compare_pmt );
				if ( pos == ev_pmts.end() )
					ev_pmts.push_back( pmt );
				else if ( pos->CompareTo( &pmt, true ) == 0 )
				{
					pos->AddData( pmt );
				}
				else
					ev_pmts.insert( pos, pmt );
				sec_len = pmt.GetBodySize(); 
				ret = SEC_OK_END;
			}
			else
				ret = SEC_NG_NEXT;
		}
		break;
	}
	if ( sec_len && p_sec_buf->GetData().size() > sec_len + 4 )
	{
		int last_pt = sec_len + 4;
		VECT_UINT8::const_iterator it = p_sec_buf->GetData().begin() + last_pt;
		if ( *it != 0xFF )
		{
			// �����ꍇ�B
			VECT_UINT8	dmy_buf;
			dmy_buf.push_back( 0 );			// �|�C���^�[�t�B�[���h��擪�ɑ}��
			std::copy( it, p_sec_buf->GetData().end(), back_inserter(dmy_buf) );
			p_sec_buf->ClearData();
			p_sec_buf->AddData( packet.header.index, &dmy_buf[0], &dmy_buf[0] + dmy_buf.size() );
			return SEC_OK_RETRY;
		}
		else
			int iii = 0;
	}
	return ret;
}

// �C�x���g�啪�ޕ�����擾
static const TCHAR*	_get_genre_lvl1_str(int genre)
{
	static const TCHAR *c_lvl1_str[] = {
		_T("�j���[�X�^��"),			// 0x0  
		_T("�X�|�[�c"),					// 0x1  
		_T("���^���C�h�V���["),		// 0x2  
		_T("�h���}"),					// 0x3  
		_T("���y"),						// 0x4  
		_T("�o���G�e�B"),				// 0x5  
		_T("�f��"),						// 0x6  
		_T("�A�j���^���B"),				// 0x7  
		_T("�h�L�������^���[�^���{"),	// 0x8  
		_T("����^����"),				// 0x9  
		_T("��^����"),				// 0xA  
		_T("����"),						// 0xB  
		_T("�\��"),						// 0xC  
		_T("�\��"),						// 0xD  
		_T("�g��"),						// 0xE  
		_T("���̑� "),					// 0xF  

		_T("�W�������w��Ȃ�"),				
	};
	_ASSERT( sizeof(c_lvl1_str) / sizeof(c_lvl1_str[0]) == 0x11 );
	if ( genre < 0 )
		return c_lvl1_str[0x10];
	else
		return c_lvl1_str[(genre>>4)&0xF];
}

// �C�x���g�����ޕ�����擾
static const TCHAR*	_get_genre_lvl2_str(int genre)
{
	static const TCHAR *c_lvl2_str[] = {
		_T("�莞�E����"),					// 0x0 0x0  
		_T("�V�C"),							// 0x0 0x1  
		_T("���W�E�h�L�������g"),			// 0x0 0x2  
		_T("�����E����"),					// 0x0 0x3  
		_T("�o�ρE�s��"),					// 0x0  0x4  
		_T("�C�O�E����"),					// 0x0  0x5  
		_T("���"),							// 0x0  0x6  
		_T("���_�E��k"),					// 0x0  0x7  
		_T("�񓹓���"),						// 0x0  0x8  
		_T("���[�J���E�n��"),				// 0x0  0x9  
		_T("���"),							// 0x0  0xA  
		_T(""),								// 0x0  0xB   
		_T(""),								// 0x0  0xC   
		_T(""),								// 0x0  0xD   
		_T(""),								// 0x0  0xE   
		_T("���̑�"),						// 0x0  0xF  

		_T("�X�|�[�c�j���[�X"),				// 0x1 0x0  
		_T("�싅"),							// 0x1 0x1  
		_T("�T�b�J�["),						// 0x1 0x2  
		_T("�S���t"),						// 0x1 0x3  
		_T("���̑��̋��Z"),					// 0x1 0x4  
		_T("���o�E�i���Z"),					// 0x1 0x5  
		_T("�I�����s�b�N�E���ۑ��"),		// 0x1 0x6  
		_T("�}���\���E����E���j"),			// 0x1 0x7  
		_T("���[�^�[�X�|�[�c"),				// 0x1 0x8  
		_T("�}�����E�E�B���^�[�X�|�[�c"),	// 0x1 0x9  
		_T("���n�E���c���Z"),				// 0x1 0xA  
		_T(""),								// 0x1 0xB   
		_T(""),								// 0x1 0xC   
		_T(""),								// 0x1 0xD   
		_T(""),								// 0x1 0xE   
		_T("���̑�"),						// 0x1 0xF  

		_T("�|�\�E���C�h�V���["),			// 0x2 0x0  
		_T("�t�@�b�V����"),					// 0x2 0x1  
		_T("��炵�E�Z�܂�"),				// 0x2  0x2  
		_T("���N�E���"),					// 0x2  0x3  
		_T("�V���b�s���O�E�ʔ�"),			// 0x2  0x4  
		_T("�O�����E����"),					// 0x2  0x5  
		_T("�C�x���g"),						// 0x2  0x6  
		_T("�ԑg�Љ�E���m�点"),			// 0x2  0x7  
		_T(""),								// 0x2 0x8   
		_T(""),								// 0x2 0x9   
		_T(""),								// 0x2 0xA   
		_T(""),								// 0x2 0xB   
		_T(""),								// 0x2 0xC   
		_T(""),								// 0x2 0xD   
		_T(""),								// 0x2 0xE   
		_T("���̑�"),						// 0x2 0xF  

		_T("�����h���}"),					// 0x3  0x0  
		_T("�C�O�h���}"),					// 0x3  0x1  
		_T("���㌀"),						// 0x3  0x2  
		_T(""),								// 0x3  0x3   
		_T(""),								// 0x3  0x4   
		_T(""),								// 0x3  0x5   
		_T(""),								// 0x3  0x6   
		_T(""),								// 0x3  0x7   
		_T(""),								// 0x3  0x8   
		_T(""),								// 0x3  0x9   
		_T(""),								// 0x3  0xA   
		_T(""),								// 0x3  0xB   
		_T(""),								// 0x3  0xC   
		_T(""),								// 0x3  0xD   
		_T(""),								// 0x3  0xE   
		_T("���̑�"),						// 0x3  0xF  

		_T("�������b�N�E�|�b�v�X"),			// 0x4 0x0  
		_T("�C�O���b�N�E�|�b�v�X"),			// 0x4 0x1  
		_T("�N���V�b�N�E�I�y��"),			// 0x4 0x2  
		_T("�W���Y�E�t���[�W����"),			// 0x4 0x3  
		_T("�̗w�ȁE����"),					// 0x4 0x4  
		_T("���C�u�E�R���T�[�g"),			// 0x4 0x5  
		_T("�����L���O�E���N�G�X�g"),		// 0x4 0x6  
		_T("�J���I�P�E�̂ǎ���"),			// 0x4 0x7  
		_T("���w�E�M�y"),					// 0x4 0x8  
		_T("���w�E�L�b�Y"),					// 0x4 0x9  
		_T("�������y�E���[���h�~���[�W�b�N"),	// 0x4 0xA  
		_T(""),									// 0x4 0xB   
		_T(""),									// 0x4 0xC   
		_T(""),									// 0x4 0xD   
		_T(""),									// 0x4 0xE   
		_T("���̑�"),							// 0x4 0xF  

		_T("�N�C�Y"),							// 0x5  0x0  
		_T("�Q�[��"),							// 0x5  0x1  
		_T("�g�[�N�o���G�e�B"),					// 0x5  0x2  
		_T("���΂��E�R���f�B"),					// 0x5  0x3  
		_T("���y�o���G�e�B"),					// 0x5  0x4  
		_T("���o���G�e�B"),						// 0x5  0x5  
		_T("�����o���G�e�B"),					// 0x5  0x6  
		_T(""),									// 0x5  0x7   
		_T(""),									// 0x5  0x8   
		_T(""),									// 0x5  0x9   
		_T(""),									// 0x5  0xA   
		_T(""),									// 0x5  0xB   
		_T(""),									// 0x5  0xC   
		_T(""),									// 0x5  0xD   
		_T(""),									// 0x5  0xE   
		_T("���̑�"),							// 0x5  0xF  

		_T("�m��"),								// 0x6 0x0  
		_T("�M��"),								// 0x6 0x1  
		_T("�A�j��"),							// 0x6 0x2  
		_T(""),									// 0x6 0x3   
		_T(""),									// 0x6 0x4   
		_T(""),									// 0x6 0x5   
		_T(""),									// 0x6 0x6   
		_T(""),									// 0x6 0x7   
		_T(""),									// 0x6 0x8   
		_T(""),									// 0x6 0x9   
		_T(""),									// 0x6 0xA   
		_T(""),									// 0x6 0xB   
		_T(""),									// 0x6 0xC   
		_T(""),									// 0x6 0xD   
		_T(""),									// 0x6 0xE   
		_T("���̑�"),							// 0x6 0xF  

		_T("�����A�j��"),		// 0x7  0x0  
		_T("�C�O�A�j��"),		// 0x7  0x1  
		_T("���B"),		// 0x7  0x2  
		_T(""),		// 0x7  0x3   
		_T(""),		// 0x7  0x4   
		_T(""),		// 0x7  0x5   
		_T(""),		// 0x7  0x6   
		_T(""),		// 0x7  0x7   
		_T(""),		// 0x7  0x8   
		_T(""),		// 0x7  0x9   
		_T(""),		// 0x7  0xA   
		_T(""),		// 0x7  0xB   
		_T(""),		// 0x7  0xC   
		_T(""),		// 0x7  0xD   
		_T(""),		// 0x7  0xE   
		_T("���̑�"),		// 0x7  0xF  

		_T("�Љ�E����"),		// 0x8 0x0  
		_T("���j�E�I�s"),		// 0x8 0x1  
		_T("���R�E�����E��"),		// 0x8 0x2  
		_T("�F���E�Ȋw�E��w"),		// 0x8 0x3  
		_T("�J���`���[�E�`������"),		// 0x8 0x4  
		_T("���w�E���|"),		// 0x8 0x5  
		_T("�X�|�[�c"),		// 0x8 0x6  
		_T("�h�L�������^���[�S��"),		// 0x8 0x7  
		_T("�C���^�r���[�E���_"),		// 0x8 0x8  
		_T(""),		// 0x8 0x9   
		_T(""),		// 0x8 0xA   
		_T(""),		// 0x8 0xB   
		_T(""),		// 0x8 0xC   
		_T(""),		// 0x8 0xD   
		_T(""),		// 0x8 0xE   
		_T("���̑�"),		// 0x8 0xF  

		_T("���㌀�E�V��"),		// 0x9  0x0  
		_T("�~���[�W�J��"),		// 0x9  0x1  
		_T("�_���X�E�o���G"),		// 0x9  0x2  
		_T("����E���|"),		// 0x9  0x3  
		_T("�̕���E�ÓT"),		// 0x9  0x4  
		_T(""),		// 0x9  0x5   
		_T(""),		// 0x9  0x6   
		_T(""),		// 0x9  0x7   
		_T(""),		// 0x9  0x8   
		_T(""),		// 0x9  0x9   
		_T(""),		// 0x9  0xA   
		_T(""),		// 0x9  0xB   
		_T(""),		// 0x9  0xC   
		_T(""),		// 0x9  0xD   
		_T(""),		// 0x9  0xE   
		_T("���̑�"),		// 0x9  0xF  

		_T("���E�ނ�E�A�E�g�h�A"),		// 0xA 0x0  
		_T("���|�E�y�b�g�E��|"),		// 0xA 0x1  
		_T("���y�E���p�E�H�|"),		// 0xA 0x2  
		_T("�͌�E����"),		// 0xA 0x3  
		_T("�����E�p�`���R"),		// 0xA 0x4  
		_T("�ԁE�I�[�g�o�C"),		// 0xA 0x5  
		_T("�R���s���[�^�E�s�u�Q�[��"),		// 0xA 0x6  
		_T("��b�E��w"),		// 0xA 0x7  
		_T("�c���E���w��"),		// 0xA 0x8  
		_T("���w���E���Z��"),		// 0xA 0x9  
		_T("��w���E��"),		// 0xA 0xA  
		_T("���U����E���i"),		// 0xA 0xB  
		_T("������"),		// 0xA 0xC  
		_T(""),		// 0xA 0xD   
		_T(""),		// 0xA 0xE   
		_T("���̑�"),		// 0xA 0xF  
  
		_T("�����"),		// 0xB  0x0  
		_T("��Q��"),		// 0xB  0x1  
		_T("�Љ��"),		// 0xB  0x2  
		_T("�{�����e�B�A"),		// 0xB  0x3  
		_T("��b"),		// 0xB  0x4  
		_T("�����i�����j"),		// 0xB  0x5  
		_T("�������"),		// 0xB  0x6  
		_T(""),		// 0xB  0x7   
		_T(""),		// 0xB  0x8   
		_T(""),		// 0xB  0x9   
		_T(""),		// 0xB  0xA   
		_T(""),		// 0xB  0xB   
		_T(""),		// 0xB  0xC   
		_T(""),		// 0xB  0xD   
		_T(""),		// 0xB  0xE   
		_T("���̑�"),		// 0xB  0xF  

		_T(""),		// 0xC 0x0   
		_T(""),		// 0xC 0x1   
		_T(""),		// 0xC 0x2   
		_T(""),		// 0xC 0x3   
		_T(""),		// 0xC 0x4   
		_T(""),		// 0xC 0x5   
		_T(""),		// 0xC 0x6   
		_T(""),		// 0xC 0x7   
		_T(""),		// 0xC 0x8   
		_T(""),		// 0xC 0x9   
		_T(""),		// 0xC 0xA   
		_T(""),		// 0xC 0xB   
		_T(""),		// 0xC 0xC   
		_T(""),		// 0xC 0xD   
		_T(""),		// 0xC 0xE   
		_T(""),		// 0xC 0xF   

		_T(""),		// 0xD 0x0   
		_T(""),		// 0xD 0x1   
		_T(""),		// 0xD 0x2   
		_T(""),		// 0xD 0x3   
		_T(""),		// 0xD 0x4   
		_T(""),		// 0xD 0x5   
		_T(""),		// 0xD 0x6   
		_T(""),		// 0xD 0x7   
		_T(""),		// 0xD 0x8   
		_T(""),		// 0xD 0x9   
		_T(""),		// 0xD 0xA   
		_T(""),		// 0xD 0xB   
		_T(""),		// 0xD 0xC   
		_T(""),		// 0xD 0xD   
		_T(""),		// 0xD 0xE   
		_T(""),		// 0xD 0xF   

		_T("BS/�n��f�W�^�������p�ԑg�t�����"),		// 0xE 0x0  
		_T("�L�ш� CS�f�W�^�������p�g��"),		// 0xE 0x1  
		_T("�q���f�W�^�����������p�g��"),		// 0xE 0x2  
		_T("�T�[�o�[�^�ԑg�t�����"),		// 0xE 0x3  
		_T("IP�����p�ԑg�t�����"),		// 0xE 0x4  
		_T(""),		// 0xE 0x5   
		_T(""),		// 0xE 0x6   
		_T(""),		// 0xE 0x7   
		_T(""),		// 0xE 0x8   
		_T(""),		// 0xE 0x9   
		_T(""),		// 0xE 0xA   
		_T(""),		// 0xE 0xB   
		_T(""),		// 0xE 0xC   
		_T(""),		// 0xE 0xD   
		_T(""),		// 0xE 0xE   
		_T(""),		// 0xE 0xF   

		_T(""),		// 0xF  0x0   
		_T(""),		// 0xF  0x1   
		_T(""),		// 0xF  0x2   
		_T(""),		// 0xF  0x3   
		_T(""),		// 0xF  0x4   
		_T(""),		// 0xF  0x5   
		_T(""),		// 0xF  0x6   
		_T(""),		// 0xF  0x7   
		_T(""),		// 0xF  0x8   
		_T(""),		// 0xF  0x9   
		_T(""),		// 0xF  0xA   
		_T(""),		// 0xF  0xB   
		_T(""),		// 0xF  0xC   
		_T(""),		// 0xF  0xD   
		_T(""),		// 0xF  0xE   
		_T("���̑�"),		// 0xF  0xF  
	};
	static const TCHAR *c_undefined_str = _T("����`");
	static const TCHAR *c_unassign_str = _T("");
	
	_ASSERT( sizeof(c_lvl2_str) / sizeof(c_lvl2_str[0]) == 0x100 );
	if ( genre < 0 )
		return c_unassign_str;
	else
	{
		const TCHAR *pret = c_lvl2_str[genre&0xFF];
		if ( !*pret || !pret )			// ""�̏ꍇ�A����`
			return c_undefined_str;
		else
			return pret;
	}
}

static TCHAR *_get_datetime_str(time_t tt)
{
	static TCHAR	sz_tbuf[64];

	_tcsftime( sz_tbuf, 64, _T("%Y/%m/%d %H:%M:%S"), localtime(&tt) );

	return sz_tbuf;
}

static UINT64 _make_key( int network_id, int transport_id, int service_id)
{
	UINT64	key = network_id;
	key <<= 16;
	key |= transport_id;
	key <<= 16;
	key |= service_id;
	return key;
}

// SDT���T�[�r�X��T���B
// ������Ȃ������Ƃ�
//		SDT�ɊY�����ڂ��Ȃ�			��ev_psdt�̍ŏ��̍��ڂ�services.end()
//		services�z����ɍ��ڂ��Ȃ�	��ev_psdt�̍Ō�̍��ڂ�services.end()
// ev_psdts�̍��ڐ�0�łȂ����ƑO��
static bool  _find_sdt_svc(int network_id, int transport_id, int service_id, CSDT::VECT_SERV::iterator &it)
{
	CSDT	key;
	key.id					= transport_id;
	key.original_network_id = network_id;
	key.table_id			= -1;		// network_id,id(transport_id)���������ŏ��̍��ڂ�Ԃ��悤�ɂ��邽�߁BCSDT::CompareTo()�̎����ˑ��Ȃ�ł��܂��낵���Ȃ�
	
	vector<CSDT*>::iterator p = lower_bound( ev_psdts.begin(), ev_psdts.end(), &key, _compare_sdt );
	if ( p == ev_psdts.end() || (*p)->id != transport_id || (*p)->original_network_id != network_id )
	{
		return false;
	}
	for ( ; p != ev_psdts.end(); ++p )
	{
		CSDT *psdt = *p;
		if ( psdt->original_network_id != network_id || psdt->id != transport_id )
			break;
		for ( CSDT::VECT_SERV::iterator is = psdt->services.begin(); is != psdt->services.end(); ++is )
		{
			if ( is->service_id == service_id )
			{
				// �����B�����������ꍇ�A���SDT�������������������H�H
				it = is;
				return true;
			}
		}
	}
	return false;
}

static SServiceInfo*	_make_program_info(MAP_SERV	&services, const CEIT *peit, time_t tot_start, time_t tot_end)
{
	SServiceInfo	*psvc;
	{
		UINT64 map_key = _make_key( peit->original_network_id, peit->transport_stream_id, peit->id );
		MAP_SERV::_Pairib pib = services.insert( MAP_SERV::value_type( map_key, SServiceInfo() ) );

		psvc = &pib.first->second;
		if ( pib.second )
		{	// �V�K�o�^��
			// �����o������
			psvc->network_id		= peit->original_network_id;
			psvc->transport_id	= peit->transport_stream_id;
			psvc->service_id		= peit->id;
			
			// �Y���T�[�r�X��SDT���������A���O�Ȃǂ��Z�b�g����
			CSDT::VECT_SERV::iterator is;
			if ( !_find_sdt_svc( psvc->network_id, psvc->transport_id, psvc->service_id, is ) )
			{
				DBG_LOG_ASSERT( _T("\rnetwork_id:%d trans_id:%d svc_id:%d��SDT/Service���Ȃ�\n"), psvc->network_id, psvc->transport_id, psvc->service_id );
			}
			else
			{
				for ( VECT_DESC::iterator iv = is->descriptors.begin(); iv != is->descriptors.end(); ++iv )
				{
					switch ( iv->tag )
					{
					case DSCTAG_SERVICE:
						{
							CServiceDesc desc(*iv);
							psvc->prov_name = desc.service_provider_name;
							psvc->name = desc.service_name;
							psvc->service_type = desc.service_type;
							goto end_serv_find_loop;
						}
					}
				}
end_serv_find_loop:
				;
			}
		}
	}
	for ( vector<CEIT::CEvent>::const_iterator ie = peit->event_tbl.begin(); ie != peit->event_tbl.end(); ie++ )
	{
		bool	b_add_prg = false;
		time_t	t_start, t_end;
		tm		tm_b;

		_convert_time_jst40( ie->start_time, tm_b );
		t_start = mktime( &tm_b );
		_convert_time_bcd( ie->duration, tm_b );
		t_end = t_start + tm_b.tm_hour * 3600 + tm_b.tm_min * 60 + tm_b.tm_sec;

		if ( ev_opt_is_all_out )
			b_add_prg = true;
		else
		{
			// tot_start�`tot_end�̊��Ԃ�t_start�At_end���͂����Ă邩
			if ( t_start < tot_start )
				b_add_prg = tot_start <= t_end;
			else
				b_add_prg = t_start <= tot_end;
		}

		if ( b_add_prg )
		{
			SProgramInfo *pprg;
			{
				MAP_PRG::_Pairib pib = psvc->programs.insert( MAP_PRG::value_type( ie->event_id, SProgramInfo() ) );
				pprg = &pib.first->second;
				if ( pib.second )
				{	// �V�K�o�^���A�����o��������
					pprg->event_id = ie->event_id;
					pprg->start_time = t_start;
					pprg->end_time = t_end;
					pprg->genre = -1;
					pprg->flg = 0;
					pprg->stream_content = -1;
					pprg->component_type = -1;
					pprg->ext_evt_flg = 0;
				}
			}
			const int prg_all_flg = 4;
			// �X�P�W���[���n�ƌ���/���̔ԑg�n��event_id���d������\���L��B
			// ������Z�b�g����ƒx����������Ȃ���ŁA���ɐݒ肵�����̓Z�b�g���Ȃ��悤�ɂ���
			// ���X�Q�񂵂��Z�b�g���Ȃ�(��)�Ȃ̂ŁA�C�ɂ��Ȃ��Ă�������������Ȃ�(^_^;
			if ( pprg->flg ^ ((1<<prg_all_flg)-1) )
			{
				for ( VECT_DESC::const_iterator id = ie->descriptors.begin(); id != ie->descriptors.end(); id++ )
				{
					try
					{
						switch ( id->tag )
						{
						case DSCTAG_SHORTEVT:
							if ( !(pprg->flg & (1<<0)) )
							{
								CShortEventDesc	dsc(*id);

								pprg->name_str = dsc.name_str;
								pprg->desc_str = dsc.desc_str;
								if ( dsc.name_str.length() > 0 && dsc.desc_str.length() > 0 )
									pprg->flg |= 1<<0;
							}
							break;
						case DSCTAG_CONTENT:
							if ( !(pprg->flg & (1<<1)) )
							{
								CContentDesc dsc(*id);

								pprg->genre = (dsc.content_nibble_level_1<<4) | dsc.content_nibble_level_2;
								pprg->flg |= 1<<1;
							}
							break;
						case DSCTAG_COMPONENT:
							if ( !(pprg->flg & (1<<2)) )
							{
								CComponentDesc dsc(*id);

								pprg->stream_content = dsc.stream_content;
								pprg->component_type = dsc.component_type;
								pprg->flg |= 1<<2;
							}
							break;
						case DSCTAG_EXTEVT:
							if ( !(pprg->flg & (1<<3)) )
							{
								CExtEventDesc	dsc(*id);
								static VECT_UINT8 *st_p_prev_dat = NULL;		// ���ږ��󔒂̊g���L�q�͂��̑O�̍��ڂ���̋L�q����Ȃ���݂����B
																				// static�ɂ����for���[�v�̊O�Ő錾���Ƃ��Ηǂ��悤�ȋC������B

								for ( CExtEventDesc::VECT_ITEM::iterator ix = dsc.items.begin(); ix != dsc.items.end(); ++ix )
								{
									if ( (*ix).description.length() > 0 || !st_p_prev_dat )
									{
										CNTR_EXTEV::_Pairib pib = pprg->ext_evts.insert( CNTR_EXTEV::value_type((*ix).description, VECT_UINT8()) );
										VECT_UINT8 &data = (*pib.first).second;
										// ���ڋL�q�𖖔��ɒǉ�
										copy( (*ix).text.begin(), (*ix).text.end(), back_inserter(data) );
										st_p_prev_dat = &data;									
									}
									else
									{
										// �O�̍��ڋL�q�̖����ɒǉ�
										copy( (*ix).text.begin(), (*ix).text.end(), back_inserter(*st_p_prev_dat) );
									}
								}
								if ( dsc.text.size() > 0 )
								{
									CNTR_EXTEV::_Pairib pib = pprg->ext_evts.insert( CNTR_EXTEV::value_type(_T(""), VECT_UINT8()) );
									VECT_UINT8 &data = (*pib.first).second;

									copy( dsc.text.begin(), dsc.text.end(), back_inserter(data) );
								}
								pprg->ext_evt_flg |= (1<<dsc.descriptor_number);
								// �S���̊g���`���C�x���g�L�q�q�����������獀�ڃZ�b�g�r�b�g�𗧂Ă�B
								// �����C�x���gID�ŕʓ��e�̊g���`���C�x���g�L�q�q��������A����͖��������(�����̂��ȁH)
								if ( !(pprg->ext_evt_flg ^ ((1<<(dsc.last_descriptor_number+1))-1)) )
								{
									pprg->flg |= 1<<3;
									st_p_prev_dat = NULL;
								}
							}
							break;
						// ���̋L�q�q��ǉ�����ꍇ�Aprg_all_flg�𑝂₵�āApprg->flg�ŗ��Ă�r�b�g�𑼂Əd�����Ȃ��悤�ɂ��邱��
						}
					}
					catch (CDefExcept &d)
					{
						DBG_LOG_ASSERT( _T("\rservice:%d event:%d dscidx:%d %s\n"), 
							psvc->service_id, pprg->event_id, id - ie->descriptors.begin(), d.str_msg );
					}
				}
			}
		}
	}
	return psvc;
}

static bool _compare_prg(const MAP_PRG::const_iterator &it1, const MAP_PRG::const_iterator &it2)
{
	return it1->second.start_time < it2->second.start_time;
}

// ���̍\��
// lpszFileName			�o�̓t�@�C����
// time_offset_out		TOT�o�͎��ɃI�t�Z�b�g�e�����󂯂�H
// offset_beg			�J�n���ԃI�t�Z�b�g(�b)
// offset_end			�I�����ԃI�t�Z�b�g(�b)
static int _output_program_info(tofstream &ofs)
{
	MAP_SERV		services;
	// �ʏ�TIME_JST40�̔�r�����ł�����Ǝv�����A0:00���ׂ��Ƃ��������Ȃ肻���Ȃ̂ŁAtime_t�Ŏ��Ԕ�r����
	time_t		tot_start, tot_end;		

	if		( ev_pats.size() == 0 )
		return ERR_NOPAT;
	else if ( ev_tots.size() < 2 )
		return ERR_NOTOT;	
	else if ( ev_psdts.size() == 0 )
		return ERR_NOSDT;

	tot_start = ev_tots[0].m_time;
	tot_end = ev_tots[1].m_time;

	if ( tot_start + ev_opt_offset_beg > tot_end - ev_opt_offset_end )
	{	// TS�b�������Ȃ�����ꍇ�Astart/end�̒��Ԓl�����
		time_t t_dummy = (tot_start+tot_end)/2;
		tot_start = tot_end = t_dummy;
	}
	else
	{
		tot_start += ev_opt_offset_beg;
		tot_end -= ev_opt_offset_end;
	}

	clock_t clk_st = clock();

	if ( ev_opt_is_all_out )
	{
		for ( vector<CEIT*>::iterator ie = ev_peits.begin(); ie != ev_peits.end(); ++ie )
			_make_program_info( services, *ie, tot_start, tot_end );
	}
	else
	{
		for ( vector<CPAT>::iterator ip = ev_pats.begin(); ip != ev_pats.end(); ip++ )
		{
			for ( VECT_UINT16::iterator ipp = ip->prg_nos.begin(); ipp != ip->prg_nos.end(); ipp++ )
			{
				vector<CEIT*>::iterator pos;
				SServiceInfo *psvc;
				{
					CEIT	key;
					key.transport_stream_id = ip->id;
					key.id					= *ipp;
					key.original_network_id	= 0;			// transport_id,id(service_id)���������ŏ��̍��ڂ�Ԃ��悤�ɂ��邽�߁BCEIT::CompareTo()�̎����ˑ��Ȃ�ł��܂��낵���Ȃ�
					pos = lower_bound(ev_peits.begin(), ev_peits.end(), &key, _compare_eit );
					if ( pos == ev_peits.end() || (*pos)->id != key.id )
					{	
						// SDT���T�[�r�X����������EIT���Ȃ��̂��������̂����ꉞ�m�F�Bnetwork_id�͓K���ɃZ�b�g
						CSDT::VECT_SERV::iterator p;
						bool flg = false;
						if ( _find_sdt_svc( ev_peits.front()->original_network_id, key.transport_stream_id, key.id, p ) )
						{
							if ( !p->eit_present_following_flag && (!ev_opt_read_eit_sch || !p->eit_schedule_flag) )
								flg = true;
						}
						if ( !flg )
							DBG_LOG_ASSERT( _T("\rtrans_id:%d svc_id:%d��SDT���Ȃ��̂ŁAEIT���݂��Ȃ��̂����������ǂ������ʂł��Ȃ�\n"), ip->id, *ipp );
						continue;
					}
				}
				do
				{
					psvc = _make_program_info( services, *pos, tot_start, tot_end );
					pos++;
				}	
				while (pos != ev_peits.end()
					&&	(*pos)->id == psvc->service_id 
					&&  (*pos)->transport_stream_id == psvc->transport_id
					&&	(*pos)->original_network_id == psvc->network_id );
			}
		}
		// SProgramInfo���Ȃ�SServiceInfo���폜����
		{
			MAP_SERV::iterator p = services.begin();
			while ( p != services.end() )
			{
				if ( p->second.programs.size() == 0 )
					p = services.erase( p );
				else
					p++;
			}
		}
	}
	//DBG_LOG_WARN( _T("make_prg:%.2f\n"), static_cast<double>(clock() - clk_st) / CLOCKS_PER_SEC );
	//----------

	clk_st = clock();
#ifdef _TEST_USE_FS_
	tofstream&	tss = ofs;
#else
	tstrstream tss;
#endif
	//----------
	tss << _T("TOT");
	if ( !ev_opt_outtime_offset )
	{
		tss << _T("\t") << _get_datetime_str(ev_tots[0].m_time);
		tss << _T("\t") << _get_datetime_str(ev_tots[1].m_time);
	}
	else
	{
		tss << _T("\t") << _get_datetime_str(tot_start);
		tss << _T("\t") << _get_datetime_str(tot_end);
	}
	tss << endl;
	if ( ev_opt_is_out_patpmt )
	{
		for ( vector<CPAT>::iterator ip = ev_pats.begin(); ip != ev_pats.end(); ip++ )
		{
			_ASSERT( ip->prg_nos.size() == ip->prg_map_pids.size() );
			for ( size_t i = 0; i < ip->prg_nos.size(); i++ )
			{
				tss << _T("PAT");
				tss << _T("\t") << ip - ev_pats.begin();
				tss << _T("\t") << ip->id;
				tss << _T("\t") << i;
				tss << _T("\t") << ip->prg_nos[i];
				tss << _T("\t") << ip->prg_map_pids[i];
				tss << endl;
			}
		}
		int i = 0;
		for ( vector<CPMT>::iterator im = ev_pmts.begin(); im != ev_pmts.end(); im++ )
		{
			for ( CPMT::VECT_STRM::iterator is = im->stream_tbl.begin(); is != im->stream_tbl.end(); is++ )
			{
				tss << _T("PMT1");
				tss << _T("\t") << i;
				tss << _T("\t") << im->m_self_pid;
				tss << _T("\t") << im->id;
				tss << _T("\t") << is - im->stream_tbl.begin();
				tss << _T("\t") << is->stream_type;
				tss << _T("\t") << is->elementary_pid;
				tss << endl;
			}
			tss << _T("PMT2");
			tss << _T("\t") << i;
			tss << _T("\t") << im->m_self_pid;
			tss << _T("\t") << im->id;
			tss << _T("\t") << im->m_file_pos;
			tss << _T("\t") << im->pcr_pid;
			tss << endl;
			i++;
		}
	}

	for ( MAP_SERV::iterator im = services.begin(); im != services.end(); im++ )
	{
		SServiceInfo &ssi = im->second;
		tss << _T("SVC");
		tss << _T("\t") << ssi.network_id;
		tss << _T("\t") << ssi.transport_id;
		tss << _T("\t") << ssi.service_id;
		switch ( ssi.network_id )
		{
		case 4:		// BS�f�W�^��
			tss << _T("\tBS");	break;
		case 1:		// PerfecTV!	
		case 3:		// SKY
		case 6:		// �X�J�p�[ e2(CS1)
		case 7:		// �X�J�p�[ e2(CS2)
		case 10:	// �X�J�p�[ HD
			tss << _T("\tCS");	break;
		default:
			if ( 0x7880 <= ssi.network_id && ssi.network_id <= 0x7FE8 )
				tss << _T("\t�n�f");
			else
				tss << _T("\t���̑�");
			break;
		}
		tss << _T("\t") << ssi.name.c_str();
		tss << _T("\t") << ssi.prov_name.c_str();
		tss << _T("\t") << ssi.service_type;
		tss << endl;

		// �J�n���ԂŃ\�[�g����
		vector<MAP_PRG::const_iterator>	sort_prgmap;
		for ( MAP_PRG::const_iterator ip = ssi.programs.begin(); ip != ssi.programs.end(); ip++ )
			sort_prgmap.push_back( ip );
		sort( sort_prgmap.begin(), sort_prgmap.end(), _compare_prg );

		//for ( MAP_PRG::iterator ip = ssi.programs.begin(); ip != ssi.programs.end(); ip++ )
		for ( vector<MAP_PRG::const_iterator>::iterator it = sort_prgmap.begin(); it != sort_prgmap.end(); ++it )
		{
			//SProgramInfo &spi = ip->second;
			const SProgramInfo &spi = (*it)->second;

			tss << _T("EVT");
			tss << _T("\t") << ssi.service_id;
			tss << _T("\t") << spi.event_id;
			tss << _T("\t") << _get_datetime_str(spi.start_time);
			tss << _T("\t") << _get_datetime_str(spi.end_time);
			tss << _T("\t");
			tss.write( spi.name_str.c_str(), spi.name_str.length() );
			tss << _T("\t") << spi.genre;
			tss << _T("\t") << _get_genre_lvl1_str(spi.genre);
			tss << _T("\t") << _get_genre_lvl2_str(spi.genre);
			if ( ev_opt_evt_output_type >= 1 )
			{
				if ( spi.stream_content == 1 || spi.stream_content == 5 )
				{
					switch ( (spi.component_type >> 4) & 0xf )
					{
					case 0x0:	tss << _T("\t480i");	break;
					case 0x9:	tss << _T("\t2160p");	break;
					case 0xA:	tss << _T("\t480p");	break;
					case 0xB:	tss << _T("\t1080i");	break;
					case 0xC:	tss << _T("\t720p");	break;
					case 0xD:	tss << _T("\t240p");	break;
					case 0xE:	tss << _T("\t1080p");	break;
					case 0xF:	tss << _T("\t180p");	break;
					default:	tss << _T("\t????");	break;
					}
				}
				else if ( spi.stream_content == -1 )
					tss << _T("\t��w��");
				else
					tss << _T("\t��f��");

				tss << _T("\t") << ((spi.stream_content<<8)|spi.component_type);
			}
			if ( ev_opt_evt_output_type >= 2 )
			{
				tss << _T("\t");
				tss.write( spi.desc_str.c_str(), spi.desc_str.length() );
			}
			tss << endl;
			if ( ev_opt_is_out_extevt )
			{
				int i = 0;
				for ( CNTR_EXTEV::const_iterator ie = spi.ext_evts.begin(); ie != spi.ext_evts.end(); ++ie )
				{
					const tstring		&name = (*ie).first;
					const VECT_UINT8	&data = (*ie).second;
					if ( name.length() > 0 || data.size() > 0 )
					{
						tstring		text;
						AribToString( &text, &data[0], data.size() );

						tss << _T("XEVT");
						tss << _T("\t") << ssi.service_id;
						tss << _T("\t") << spi.event_id;
						tss << _T("\t") << i;
						tss << _T("\t") << name.c_str();
						tss << _T("\t") << text.c_str();
						tss << endl;

						i++;
					}
				}
			}
		}
	}
#ifndef _TEST_USE_FS_
	{
		ofs << tss.rdbuf();
	}
#endif
	//DBG_LOG_WARN(_T("write_prg:%.2f\n"), static_cast<double>(clock() - clk_st) / CLOCKS_PER_SEC );

	return 0;
}

static void _dbg_out(tofstream &ofs)
{
#ifndef _TEST_USE_FS_
	tstrstream ou;
#else
	tofstream &ou = ofs;
#endif
	
	ou << _T("<<PAT>>") << endl;
	for ( vector<CPAT>::iterator p = ev_pats.begin(); p != ev_pats.end(); p++ )
	{
		ou << _T("trans_id:") << p->id << _T(" version:") << p->version << _T(" section:") << p->section << _T(" last_section:") << p->last_section << endl;
		for ( size_t i = 0; i < p->prg_nos.size(); i++ )
		{
			ou << _T("  [") << i << _T("] prg_no:") << (int)p->prg_nos[i] << _T(" pmt_pid:") << (int)p->prg_map_pids[i] << endl;
		}
		ou << endl;
	}

	ou << endl << _T("<<PMT>>") << endl;
	for ( vector<CPMT>::iterator p = ev_pmts.begin(); p != ev_pmts.end(); p++ )
	{
		ou << _T("service_id:") << p->id << _T(" pcr_pid:") << p->pcr_pid << _T(" version:") << p->version << endl;
		ou << _T("desc_tag:");
		for ( VECT_DESC::iterator d = p->descriptors.begin(); d != p->descriptors.end(); d++ )
			ou << hex << showbase << d->tag << ",";
		ou << dec << noshowbase << endl;
		int i = 0;
		for ( vector<CPMT::CStream>::iterator s = p->stream_tbl.begin(); s != p->stream_tbl.end(); s++, i++ )
		{
			ou << _T("    [") << i << _T("] stream_type:") << s->stream_type << _T(" elementary_pid:") << s->elementary_pid << endl;
			ou << _T("    desc_tag:");
			for ( VECT_DESC::iterator d = s->descriptors.begin(); d != s->descriptors.end(); d++ )
				ou << hex << showbase << d->tag << ",";
			ou << dec << noshowbase << endl;
		}
	}

	ou << endl << _T("<<SDT>>") << endl;
	for ( vector<CSDT*>::iterator sp = ev_psdts.begin(); sp != ev_psdts.end(); ++sp )
	{
		CSDT *p = *sp;

		ou << _T("trans_id:") << p->id << _T(" network_id:") << p->original_network_id;
		ou << _T(" tbl_id:") << hex << showbase << p->table_id;
		ou << _T(" version:") << dec << noshowbase << p->version << endl;
		int i = 0;
		for ( CSDT::VECT_SERV::iterator pp = p->services.begin(); pp != p->services.end(); ++pp, ++i )
		{
			ou << _T("    [") << i << _T("] service_id:") << pp->service_id << _T(" eit_sch_flg:") << pp->eit_schedule_flag;
			ou << _T(" eit_pres_flg:") << pp->eit_present_following_flag << endl;
			ou << _T("    desc_tag:");
			for ( VECT_DESC::iterator d = pp->descriptors.begin(); d != pp->descriptors.end(); d++ )
				ou << hex << showbase << d->tag << ",";
			ou << dec << noshowbase << endl;
		}
	}

	ou << endl << _T("<<TOT>>") << endl;
	for ( vector<CTOT>::iterator p = ev_tots.begin(); p != ev_tots.end(); p++ )
	{
		TCHAR	szbuf[64];
		tm		t;
		_convert_time_jst40( p->jst_time, t );
		_tcsftime( szbuf, 64, _T("%y/%m/%d %H:%M:%S"), &t );
		ou << _T("time:") << szbuf << endl;
	}

	ou << endl << _T("<<EIT>>") << endl;
	for ( vector<CEIT*>::iterator ep = ev_peits.begin(); ep != ev_peits.end(); ep++ )
	{
		CEIT *p = *ep;

		ou << _T("trans_id:") << p->transport_stream_id << _T(" service_id:") << p->id << _T(" network_id:") << p->original_network_id;
		ou << _T(" tbl_id:") << hex << showbase << p->table_id;
		ou << _T(" version:") << dec << noshowbase << p->version << endl;
		ou << _T("sections:");
		for ( set<UINT8>::const_iterator pp = p->GetProcSections().begin(); pp != p->GetProcSections().end(); pp++ )
		{
			ou << *pp << _T(",");
		}
		ou << endl;
		int i = 0;
		for ( vector<CEIT::CEvent>::iterator e = p->event_tbl.begin(); e != p->event_tbl.end(); e++, i++ )
		{
			tm		t;
			TCHAR	szbuf[64];
			_convert_time_jst40( e->start_time, t );
			_tcsftime( szbuf, 64, _T("%y/%m/%d %H:%M"), &t );
			ou << _T("  [") << i << _T("] evt_id:") << e->event_id << _T(" start_time:") << szbuf;
			_convert_time_bcd( e->duration, t );
			ou << _T(" duration:") << setfill(_T('0')) << setw(2) << t.tm_hour << _T(":") << setw(2) << t.tm_min << _T(":") << setw(2) << t.tm_sec;
			ou << setfill(_T(' ')) << endl;
			ou << _T("    desc_tag:");
			for ( VECT_DESC::iterator d = e->descriptors.begin(); d != e->descriptors.end(); d++ )
				ou << hex << showbase << d->tag << ",";
			ou << dec << noshowbase << endl;
		}
		ou << endl;
	}
#ifndef _TEST_USE_FS_
	ofs << ou.rdbuf();
#endif
}

// �f�o�b�O���O�o��
// ���̊֐��͏����n�ˑ��㓙�ō��(^_^;
void dbg_log_out(int rpt_type, TCHAR *filename, int line, TCHAR *lpsz_msg, ... )
{
	TCHAR	szmsg[4096];
	TCHAR	sfname[256], sext[256];
	va_list	va_arg;
	
	_tsplitpath( filename, NULL, NULL, sfname, sext );

	va_start( va_arg, lpsz_msg );
	_vsntprintf( szmsg, 4096, lpsz_msg, va_arg);		// vsnprintf
	va_end( va_arg );
#ifdef _UNICODE
	_RPTW4( (rpt_type==_CRT_ASSERT?_CRT_WARN:rpt_type), _T("%s%s(%d) %s"), sfname, sext, line, szmsg );
#else
	_RPT4( (rpt_type==_CRT_ASSERT?_CRT_WARN:rpt_type), _T("%s%s(%d) %s"), sfname, sext, line, szmsg );
#endif
	if ( ev_ps_dbglog )
	{
		if ( !ev_dbglog_flg )
		{	// ���̎��s�ŏ��߂Ă̏ꍇ�A�����t�@�C�������f�o�b�O���O�ɋL�q
			*ev_ps_dbglog << _T("\r") << ev_opt_input_fname << endl;
			ev_dbglog_flg = true;
		}
		switch ( rpt_type )
		{
		case _CRT_WARN:		*ev_ps_dbglog << _T("<WARN>");	break;
		case _CRT_ERROR:	*ev_ps_dbglog << _T("<ERR >");	break;
		case _CRT_ASSERT:	*ev_ps_dbglog << _T("<ASSR>");	break;
		}
		*ev_ps_dbglog << sfname << sext << _T("(") << line << _T(") ");
		*ev_ps_dbglog << szmsg;
	}
}

// ������͂ƕ\��
// �Ԃ�l:0		�G���[�Ȃ�
static int _parse_option(int argc, _TCHAR* argv[])
{
	// �������
	int			ch;
	TCHAR		*p_end;

	while ((ch = getopt(argc, argv, _T("o:r:d:l:sVnB:E:e:Txap"))) != -1)
	{
		switch ( ch )
		{
		case 'o':		// offset read
			ev_opt_offset_read = _tcstol( optarg, &p_end, 0 );
			if ( *p_end == '%' )
				ev_opt_offset_read_is_ratio = true;
			else if ( !p_end || !(*p_end) )
			{
				tcerr << _T("o�I�v�V���������𐔒l�ɕϊ��ł��܂���F") << optarg << endl;
				return ERR_PARAM;
			}
			else if ( ev_opt_offset_read < 0 )
			{
				tcerr << _T("o�I�v�V���������Ƀ}�C�i�X�̒l�͎w��ł��܂���F") << optarg << endl;
				return ERR_PARAM;
			}
			break;
		case 'r':		// max read size�w��				 
			ev_opt_max_read_size = _tcstol( optarg, &p_end, 0 );
			switch (*p_end)
			{
			case _T('%'):	ev_opt_max_read_type = RS_RATIO;	break;
			case _T('s'):
			case _T('S'):	if(ev_opt_max_read_size) ev_opt_max_read_type = RS_SEC;		break;
			case 0:			
				if ( ev_opt_max_read_size < 0 )
				{
					tcerr << _T("r�I�v�V���������Ƀ}�C�i�X�̒l�͎w��ł��܂���F") << optarg << endl;
					return ERR_PARAM;
				}
				break;
			default:
				tcerr << _T("r�I�v�V���������𐔒l�ɕϊ��ł��܂���F") << optarg << endl;
				return ERR_PARAM;
			}
			break;
		case 'd':			// �f�o�b�O�_���v
			ev_opt_dbgout_fname = optarg;
			break;
		case 'l':			// �f�o�b�O���O
			ev_opt_dbglog_fname = optarg;
			break;
		case 'V':			// �ڍ׏o��
			ev_opt_verbose = true;
			break;
		case 'n':
			ev_opt_no_progress = true;
			break;
		case 's':			// �X�P�W���[��EIT��ǂݍ���
			ev_opt_read_eit_sch = true;
			break;
		case 'T':			// TOT�o�͎��I�t�Z�b�g�l�̉e�����󂯂�
			ev_opt_outtime_offset = true;
			break;
		case 'B':			// �J�n�I�t�Z�b�g�b��
			//ev_opt_offset_beg = _ttoi(optarg);
			ev_opt_offset_beg = _tcstol( optarg, &p_end, 0 );
			if ( !p_end || !(*p_end) )
			{
				tcerr << _T("B�I�v�V���������𐔒l�ɕϊ��ł��܂���F") << optarg << endl;
				return ERR_PARAM;
			}
			else if ( ev_opt_offset_beg < 0 )
			{
				tcerr << _T("B�I�v�V���������Ƀ}�C�i�X�̒l�͎w��ł��܂���F") << optarg << endl;
				return ERR_PARAM;
			}
			break;
		case 'E':			// �I���I�t�Z�b�g�b��
			//ev_opt_offset_end = _ttoi(optarg);
			ev_opt_offset_end = _tcstol( optarg, &p_end, 0 );
			if ( !p_end || !(*p_end) )
			{
				tcerr << _T("E�I�v�V���������𐔒l�ɕϊ��ł��܂���F") << optarg << endl;
				return ERR_PARAM;
			}
			else if ( ev_opt_offset_end < 0 )
			{
				tcerr << _T("B�I�v�V���������Ƀ}�C�i�X�̒l�͎w��ł��܂���F") << optarg << endl;
				return ERR_PARAM;
			}
			break;
		case 'e':			// EVT�o�͎��̏o�͌`��
			ev_opt_evt_output_type = _ttoi(optarg);
			break;
		case 'x':			// �g���C�x���g���o��
			ev_opt_is_out_extevt = true;
			break;
		case 'p':			// PAT/PMT�o��
			ev_opt_is_out_patpmt = true;
			break;
		case 'a':			// �S�T�[�r�X/�C�x���g�o��
			ev_opt_is_all_out = true;
			break;
		}
	}

	argc -= optind;
	argv += optind;

	if ( argc > 0 )
		ev_opt_input_fname  = argv[0];
	if ( argc > 1 )
		ev_opt_output_fname = argv[1];
	if ( !ev_opt_input_fname || (!ev_opt_output_fname && !ev_opt_dbgout_fname) )
	{
		tcerr << _T("eit_txtout.exe [option] input_file output_file") << endl;
		tcerr << _T("[option]��eit_txtout.txt���Q�Ɖ�����") << endl;
		return ERR_PARAM;
	}

	if ( ev_opt_verbose )
	{	// �ڍו\���B�W���o�͂ɏo��
		tcout << _T("���̓t�@�C��               :") << ev_opt_input_fname << endl;
		tcout << _T("�o�̓t�@�C��               :") << (ev_opt_output_fname ? ev_opt_output_fname : _T("")) << endl;
		tcout << _T("�f�o�b�O�_���v(-d)         :") << (ev_opt_dbgout_fname ? ev_opt_dbgout_fname : _T("�Ȃ�")) << endl;
		tcout << _T("�f�o�b�O���O(-l)           :") << (ev_opt_dbglog_fname ? ev_opt_dbglog_fname : _T("�w��Ȃ�")) << endl;
		tcout << _T("�i����\��                 :") << ev_opt_no_progress << endl;
		tcout << _T("�ǂݍ��݃I�t�Z�b�g(-o)     :") << ev_opt_offset_read << (ev_opt_offset_read_is_ratio ? _T("%") : _T("*1000*188")) << endl;
		tcout << _T("�ǂݍ��݃T�C�Y(-r)         :") << ev_opt_max_read_size;
		switch ( ev_opt_max_read_type )
		{
		case RS_SIZE:	tcout << _T("*1000*188 byte") << endl;	break;
		case RS_RATIO:	tcout << _T("%") << endl;	break;
		case RS_SEC:	tcout << _T("sec") << endl;	break;
		}
		tcout << _T("�X�P�W���[��EIT�Ǎ�(-s)    :") << ev_opt_read_eit_sch << endl;
		tcout << _T("�J�n�I�t�Z�b�g�b��(-B)     :") << ev_opt_offset_beg << endl;
		tcout << _T("�I���I�t�Z�b�g�b��(-E)     :") << ev_opt_offset_end << endl;
		tcout << _T("EVT�`�o�͌`��(-e)          :") << ev_opt_evt_output_type << endl;
		tcout << _T("PAT��PMT���o��(-p)         :") << ev_opt_is_out_patpmt << endl;
		tcout << _T("�g���`���C�x���g�o��(-x)   :") << ev_opt_is_out_extevt << endl;
		tcout << _T("�S�T�[�r�X/�C�x���g�o��(-a):") << ev_opt_is_all_out << endl;
	}
	return 0;
}

//#define _MY_FILE_BUFFERING_
int _tmain(int argc, _TCHAR* argv[])
{
#ifdef _MY_FILE_BUFFERING_
	// �z�_�u���o�b�t�@(�H)�Ƃ��Ĉ����B���x�I�ɂ͕ς���(ToT)
	const int	c_ringbuf_bitsize = 20;				// 
	const int	c_ringbuf_size = 1 << c_ringbuf_bitsize;
	const int	c_ringbuf_mask = c_ringbuf_size-1;
	UINT8		*buf;								// ���݂̃|�C���^
	int			ring_idx;
	UINT8		*ring_mybuf;						// 
	UINT64		total_rdsize = 0;
	UINT8		tempbuf[TS_PACKET_SIZE];			// �����O�o�b�t�@�I�[�𒴂���ꍇ�Ɏg�p
#else
	UINT8		buf[TS_PACKET_SIZE];				// �t�@�C���o�b�t�@
#endif
	UINT64		read_btoffset=0;					// �Ǎ��I�t�Z�b�g�o�C�g
	UINT64		read_btsize;						// �Ǎ��o�C�g�T�C�Y(�o�C�g�P��)
	UINT64		count = 0, max_read_count = 0;		// �Ǎ��񐔁A�ő�Ǎ���(0�̎��S��) (�����p�P�b�g�P��)
	tofstream	fs_dbglog;
	FILE		*file;
	int			ret = 0;
	time_t		start_time;
	// �����I�v�V����

#if defined(_DEBUG) && defined(_MSC_VER)
	// ������Y����`�F�b�N
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
//	_CrtSetBreakAlloc( 48068 );
#endif

	// ���P�[���ݒ�
	_tsetlocale( LC_ALL, _T("") );
	std::locale loc = std::locale();
	loc = std::locale(loc, STR_STL_LOCALE, std::locale::ctype);	// ���{��B3����؂�Ȃ�
	std::locale::global(loc);

	ret = _parse_option(argc, argv );
	if ( ret )
		return ret;
	// 
	if ( ev_opt_dbglog_fname )
	{
		fs_dbglog.open(ev_opt_dbglog_fname, ios_base::out|ios_base::app);		// �ǉ����[�h�ŊJ��
		if ( !fs_dbglog.is_open() )
			ev_ps_dbglog = &tclog;
		else
		{
			ev_ps_dbglog = &fs_dbglog;
			//fs_dbglog.imbue( loc );
		}
	}
	else if ( ev_opt_verbose )
	{
		ev_ps_dbglog = &tclog;
	}
	if ( ev_ps_dbglog && ev_opt_verbose )
	{
	}

	// "S"�̓V�[�P���V�����A�N�Z�X�ɍœK���B
	// ���������Ɠ���Ȃ��ł͑��x���Ⴄ(20sec�ʁH)
	file = _tfopen( ev_opt_input_fname, _T("rbS") );
	if ( !file )
	{
		tcerr << _T("���̓t�@�C�����J���܂���:") << ev_opt_input_fname << endl;
		return ERR_INPUTFILE;
	}
	// �Ō�܂ŏ������ĊJ���܂���B�ł͔߂����̂ł����ŊJ���邩�ǂ����`�F�b�N����
	if ( ev_opt_output_fname )
	{
		tofstream	fs_out;
		fs_out.open( ev_opt_output_fname, ios::out );
		if ( !fs_out )
		{
			tcerr << _T("�o�̓t�@�C�����J���܂���:") << ev_opt_output_fname << endl;
			return ERR_OUTPUTFILE;
		}
		fs_out.close();
	}
	// ������
	// �S��NULL�ɂ��Ƃ�
	memset( ev_pSectionBuf, 0, sizeof(ev_pSectionBuf) );
	memset( ev_BefSectionFilePos, 0, sizeof(ev_BefSectionFilePos) );

	// �t�@�C���T�C�Y(�Ǎ��o�C�g�T�C�Y)���擾
	{
		INT64				beg_pos, end_pos;
		UINT64				file_btsize;
		fseek( file, 0, SEEK_END);		//�t�@�C�������ֈړ�
		end_pos = _ftelli64(file);
		//fs.clear();
		fseek( file, 0, SEEK_SET);		//�t�@�C���擪�ֈړ�
		beg_pos = _ftelli64(file);

		file_btsize = read_btsize = end_pos - beg_pos;

		if ( ev_opt_offset_read )
		{
			if ( ev_opt_offset_read_is_ratio )
			{
				read_btoffset = file_btsize * ev_opt_offset_read / 100;
				// �p�P�b�g�Ŋ���؂�鐔�ɂ���
				read_btoffset = read_btoffset / TS_PACKET_SIZE * TS_PACKET_SIZE;
			}
			else
				read_btoffset = ev_opt_offset_read * TS_PACKET_SIZE * 1000;
			if ( ev_opt_verbose )
				tcout << _T("�Ǎ��I�t�Z�b�g�o�C�g       :") << read_btoffset << endl;

			INT64	offset = read_btoffset;
			const int c_32bit_offset = 0x70000000;
			_fseeki64( file, offset, SEEK_CUR );
		}

		if ( ev_opt_max_read_size )
		{
			switch ( ev_opt_max_read_type )
			{
			case RS_RATIO:
				max_read_count = ev_opt_max_read_size * file_btsize / 100;
				// �p�P�b�g�P�ʂɂ���
				max_read_count /= TS_PACKET_SIZE;
				break;
			case RS_SIZE:
				max_read_count = ev_opt_max_read_size * 1000;
				break;
			}
		}
		read_btsize = file_btsize - read_btoffset;
		if ( max_read_count > 0 && read_btsize > max_read_count*TS_PACKET_SIZE )
			read_btsize = max_read_count*TS_PACKET_SIZE;		
		if ( ev_opt_verbose && ev_opt_max_read_type != RS_SEC )
			tcout << _T("�Ǎ��o�C�g�T�C�Y           :") << read_btsize << endl;
	}

	_ASSERT( read_btoffset % TS_PACKET_SIZE == 0 );

	start_time = time( &start_time );
#ifdef _MY_FILE_BUFFERING_
	// 
	ring_mybuf = new UINT8[c_ringbuf_size];
	fread( ring_mybuf, 1, c_ringbuf_size, file );
	ring_idx = 0;
	total_rdsize = 0;
#endif
	do
	{
		bool dump_flg = false;
		
#ifdef _MY_FILE_BUFFERING_
		if ( ring_mybuf[ring_idx] != 0x47 )
		{	// ��{�I�ɂ����ɓ����Ă��邱�Ƃ͂Ȃ��͂�(�]���G���[�Ƃ�������������Ă���̂��ȁH)
			// 0x47�ǂނ܂œǂݐi�߂�
			do
			{
				total_rdsize++;
				ring_idx++;
				ring_idx &= c_ringbuf_mask;
			}
			while ( total_rdsize < read_btsize && ring_mybuf[ring_idx] != 0x47 );
			if ( total_rdsize < read_btsize )
			{	// 1�����߂�
				total_rdsize--;
				ring_idx--;
				ring_idx &= c_ringbuf_mask;
			}
			else
				break;
		}
#else
		int in_char;
		if ( (in_char = fgetc(file)) != 0x47 )
		{	// ��{�I�ɂ����ɓ����Ă��邱�Ƃ͂Ȃ��͂�(�]���G���[�Ƃ�������������Ă���̂��ȁH)
			// 0x47�ǂނ܂œǂݐi�߂�
			while ( !feof(file) && (in_char = fgetc(file)) != 0x47 );
			// 1�����߂�
			if ( !feof(file) )
				ungetc( in_char, file );
			else
				break;
		}
		ungetc( in_char, file );
#endif
		count++;
		ev_cur_filepos = read_btoffset / TS_PACKET_SIZE + count;
		// 
#ifdef _MY_FILE_BUFFERING_
		if ( ring_idx + TS_PACKET_SIZE > c_ringbuf_size )
		{	
			// �����O�o�b�t�@�I�[�𒴂���ꍇ�͈ꎞ�o�b�t�@�ɃR�s�[���Ďg�p����
			// �ʂɖ���R�s�[���Ă��ǂ����A�x���悤�ȋC������̂�
			UINT8 *pdst = tempbuf;
			UINT8 *psrc = &ring_mybuf[ring_idx];
			int i;
			for ( i = 0; i < c_ringbuf_size -ring_idx; i++ )
				*pdst++ = *psrc++;
			_ASSERT( psrc == &ring_mybuf[c_ringbuf_size] );
			psrc = &ring_mybuf[0];
			for ( ; i < TS_PACKET_SIZE; i++ )
				*pdst++ = *psrc++;
			buf = tempbuf;
		}
		else
			buf = &ring_mybuf[ring_idx];
#else
		fread( (char*)&buf[0], 1, TS_PACKET_SIZE, file );
#endif
		int pid = ((buf[1] & 0x1F) << 8) | buf[2];
		switch ( pid )
		{
		case PID_PAT:
		case PID_EIT_0:
		case PID_EIT_1:
		case PID_EIT_2:
		case PID_TOT:
		case PID_SIT:
		case PID_SDT:
			dump_flg = true;
			break;
		default:
			// PMT����������
			if ( ev_opt_is_out_patpmt )
			{	
				// PAT/PMT�o�͎�����PMT�z��͎g�p���Ȃ��̂ŁA�������̂��ߎw��������ł̂݌���
				for ( vector<CPAT>::iterator it = ev_pats.begin(); it != ev_pats.end(); ++it )
				{
					if ( find( it->prg_map_pids.begin(), it->prg_map_pids.end(), pid ) != it->prg_map_pids.end() )
					{
						dump_flg = true;
						break;
					}
				}
			}
			break;
		}
		if ( dump_flg )
		{
			bool no_err = true;
			CPacket packet( buf, false );

			CSectionBuf *p_sec = ev_pSectionBuf[packet.header.pid];
			if	(	packet.header.sync != 0x47										// �����o�C�g�s��
				||	packet.header.trans_error										// �r�b�g��肠��
				||	packet.header.pid >= 0x0002 && packet.header.pid <= 0x000F		// ����`PID�͈�
				||	packet.header.scambling == 0x01									// ����`�X�N�����u������l
				||	(!packet.header.has_adaptation && !packet.header.has_payload)	// ����`�A�_�v�e�[�V�����t�B�[���h����l
				)
			{
				no_err = false;
				DBG_LOG_WARN( _T("\rpacket header���s�� pos:%d\n"), ev_cur_filepos );
			}
			else if ( packet.header.has_payload )
			{
				if ( packet.header.payload_start )
				{
					if ( packet.p_start_payload[0] == 0 && !p_sec )
					{
						p_sec = ev_pSectionBuf[packet.header.pid] = new CSectionBuf();
						no_err = p_sec->AddData( packet.header.index, packet.p_start_payload, packet.p_end_payload );
						
					}
					else if ( p_sec )
					{
						no_err = p_sec->AddData( packet.header.index, packet.p_start_payload+1, packet.p_end_payload );
					}
				}
				else if ( p_sec )
					no_err = p_sec->AddData( packet.header.index, packet.p_start_payload, packet.p_end_payload );
				if ( !no_err )
					DBG_LOG_ASSERT( _T("\r�Z�N�V�����o�b�t�@�ݒ�ŃG���[ pos:%d\n"), ev_cur_filepos );
				if ( !p_sec )
				{
					if ( ev_BefSectionFilePos[packet.header.pid] )
					{
						DBG_LOG_ASSERT( _T("\r�Y������Z�N�V�����f�[�^���Ȃ��Bpid:0x%X bef_pos:%d\n"), 
							packet.header.pid, ev_BefSectionFilePos[packet.header.pid] );
					}
				}
			}
			if ( p_sec )
			{
				if ( !no_err )
				{	// ���݂̃Z�N�V�����f�[�^�͎̂Ă�
					delete p_sec;
					p_sec = ev_pSectionBuf[packet.header.pid] = NULL;
				}
				else
				{
					eSECTION_RET ret;
					int				proc_cnt = 0;			// �����񐔁B�f�o�b�O�p�œ��ɐ[���Ӗ�����
					while ( (ret =  _dump_section_data( packet, p_sec, ev_opt_read_eit_sch )) == SEC_OK_RETRY )
						proc_cnt++;
					if ( ret == SEC_OK_END || p_sec->GetData().size() >= 4196 )
					{
						// �����Ɍ����ƃe�[�u�����ʖ��ɍő�o�C�g��1024/4196���قȂ邪�A�����܂ōl���Ȃ�
						if ( ret != SEC_OK_END && p_sec->GetData().size() >= 4196 )
						{
							DBG_LOG_ASSERT( _T("\r�Z�N�V�����f�[�^�T�C�Y��4196�𒴂��Ă܂��Bpid:0x%X bef_pos:%d\n"),
								packet.header.pid, ev_BefSectionFilePos[packet.header.pid] );
						}
						// ���܂ł̃p�P�b�g�ŏ����ł���ꍇ�A�����s�v�Ȃ̂Ŏ̂Ă�
						delete p_sec;
						p_sec = ev_pSectionBuf[packet.header.pid] = NULL;
						ev_BefSectionFilePos[packet.header.pid] = ev_cur_filepos;
					}
				}
			}
		}
		if ( !ev_opt_no_progress && (count & ((1<<12)-1)) == 1 )			// (count % (1<<12)) == 1
		{
			time_t cur, rest;
			UINT64 percent;

			if ( ev_opt_max_read_type == RS_SEC )
			{
				if ( ev_tots.size() < 2 )
					percent = 0;
				else
					percent = (ev_tots[1].m_time - ev_tots[0].m_time) * 10000 / ev_opt_max_read_size;
			}
			else
			{
#ifdef _MY_FILE_BUFFERING_
				percent = total_rdsize * 10000 / read_btsize;					// �������̂��ߏ�����2�ʂ܂ŋ��߂�(double�Ƃ����ƒx���Ǝv��)
#else
				UINT64 pos = _ftelli64( file );;
				_ASSERT( pos >= read_btoffset );
				percent = (pos-read_btoffset) * 10000 / read_btsize;			// �������̂��ߏ�����2�ʂ܂ŋ��߂�(double�Ƃ����ƒx���Ǝv��)
#endif
			}
			tcout << _T("\r") << setw(3) << setfill(_T(' ')) << percent / 100;
			tcout << _T(".") << setw(2) << setfill(_T('0')) << percent % 100 << _T("%");

			cur = time( &cur );
			tcout << _T(" �o��:") << setw(4) << setfill(_T(' ')) << cur - start_time << _T("s");
			if ( percent )
			{
				rest = (cur - start_time) * (10000 - percent) / percent;
				tcout << _T(" �c��:") << setw(4) << rest << _T("s");
			}
		}
#ifdef _MY_FILE_BUFFERING_
		{
			int bef_blk_idx = ring_idx >> (c_ringbuf_bitsize-1);		// �O��:0 �㔼:1
			total_rdsize += TS_PACKET_SIZE;
			ring_idx += TS_PACKET_SIZE;
			ring_idx &= c_ringbuf_mask;
			int cur_blk_idx = ring_idx >> (c_ringbuf_bitsize-1);		// �O��:0 �㔼:1
			if ( bef_blk_idx != cur_blk_idx )
			{	// �����g�p���Ȃ����̃u���b�N���t�@�C������ǂݍ���
				int idx = bef_blk_idx<<(c_ringbuf_bitsize-1);
				fread( &ring_mybuf[idx], 1, 1<<(c_ringbuf_bitsize-1), file );
			}
		}

	}while( total_rdsize < read_btsize
#else
	}while( !feof(file) 
#endif
		&& (!max_read_count || !(ev_opt_max_read_type != RS_SEC && count >= max_read_count))
		&& !(ev_opt_max_read_type == RS_SEC && ev_tots.size() >= 2 && ev_tots[1].m_time - ev_tots[0].m_time >= ev_opt_max_read_size )
	);

	fclose( file );
#ifdef _MY_FILE_BUFFERING_
	delete[] ring_mybuf;
#endif

	// ���̍\��
	if ( ev_opt_output_fname )
	{
		tofstream	ofs;
		//ofs.imbue( loc );
		ofs.open( ev_opt_output_fname, ios_base::out|ios_base::trunc );
		if ( !ofs )
		{
			tcerr << _T("�o�̓t�@�C�����J���܂���:") << ev_opt_output_fname << endl;
			ret = ERR_OUTPUTFILE;
		}
		else
		{
			ret = _output_program_info(ofs);
			//ret = _output_program_info(ev_opt_output_fname, ev_opt_outtime_offset, ev_opt_offset_beg, ev_opt_offset_end, ev_opt_evt_output_type);
		}
	}
	
	if ( ev_opt_dbgout_fname )
	{
		tofstream	fs_dbgout;
		fs_dbgout.open(ev_opt_dbgout_fname, ios_base::out|ios_base::trunc);
		if ( fs_dbgout.is_open() )
		{
			//fs_dbgout.imbue( loc );			// ���ꂵ�Ȃ��Ɠ��{�ꕶ�����o�͂���Ȃ��݂����B
			fs_dbgout << _T("filename:") << ev_opt_input_fname << endl;
			_dbg_out(fs_dbgout);
		}
		fs_dbgout.close();
	}

	if ( fs_dbglog.is_open() )
		fs_dbglog.close();

	// �I����
	for ( int i = 0; i < sizeof(ev_pSectionBuf)/sizeof(ev_pSectionBuf[0]); i++ )
	{
		if ( ev_pSectionBuf[i] )
		{
			delete ev_pSectionBuf[i];
			ev_pSectionBuf[i] = NULL;	// ���ƂȂ�NULL�Ŗ��߂Ƃ�
		}
	}
	for ( vector<CEIT*>::iterator ie = ev_peits.begin(); ie != ev_peits.end(); ++ie )
		delete *ie;
	for ( vector<CSDT*>::iterator is = ev_psdts.begin(); is != ev_psdts.end(); ++is )
		delete *is;
	{
		time_t cur = time( &cur );
		if ( cur - start_time > 10 )
			tcout << endl << _T(" �o��:") << setw(4) << setfill(_T(' ')) << cur - start_time << _T("s");
	}
	return ret;
}

