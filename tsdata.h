#ifndef _INC_TSDATA_H_
#define _INC_TSDATA_H_

#include "type_define.h"

// �Z�N�V�����f�[�^(�y�C���[�h)�o�b�t�@�����O�p
class CSectionBuf
{
public:
	CSectionBuf(){next_index = add_count = 0;}
private:
	VECT_UINT8	data;
	int			next_index;		// ���̃C���f�b�N�X
	int			add_count;
public:
	int					GetNextIndex(){return next_index;}
	const VECT_UINT8&	GetData(){return data;}
public:
	//bool AddData(int data_index, VECT_UINT8::iterator beg_pos, VECT_UINT8::iterator end_pos)
	bool AddData(int data_index, UINT8 *beg_pos, UINT8 *end_pos)
	{
		if ( data_index == next_index || data.empty() )
		{
			// VC2010�ł͒ʂ邪�A�{���͂��������������͑ʖڂ����B
			std::copy( beg_pos, end_pos, std::back_inserter( data ) );
			next_index = data_index + 1;
			next_index &= 0xF;
			add_count++;
			return true;
		}
		else
		{
			return false;
		}
	}
	void ClearData()
	{
		next_index = 0;
		add_count = 0;
		data.clear();
	}
};

// �L�[�F���ږ�  �l�F�o�C�g�z��
// (�����ŃG���R�[�h����Ɠ��e�������L�q�q�ɂ܂������Ă��ꍇ�ɂ܂���)
typedef std::map<tstring, VECT_UINT8> CNTR_EXTEV;

// �ԑg���
struct SProgramInfo
{
	int			event_id;			// �C�x���gID
	int			flg;				// ���ݒ�t���O
	tstring		name_str;			// �ԑg��
	tstring		desc_str;			// ���e
	time_t		start_time;			// �J�n����
	time_t		end_time;			// �I������
	int			genre;				// �W�������B�����l-1
	int			stream_content;		// �R���|�[�l���g���e
	int			component_type;		// 
	int			ext_evt_flg;
	CNTR_EXTEV	ext_evts;	
};

typedef std::map<int,SProgramInfo>	MAP_PRG;

struct SServiceInfo
{
	int			network_id;
	int			transport_id;
	int			service_id;
	int			service_type;
	tstring		name;
	tstring		prov_name;
	MAP_PRG		programs;			// �ԑg���B�L�[�̓C�x���gID�Ƃ���
};

typedef std::map<UINT64,SServiceInfo>	MAP_SERV;

#define PID_PAT		0x000			// PAT
#define PID_EIT_0	0x012			// EIT
#define PID_EIT_1	0x026			// EIT
#define PID_EIT_2	0x027			// EIT
#define PID_TOT		0x014			// TOT/TDT
#define PID_SIT		0x01F
//#define PID_NIT		0x010			// NIT
#define PID_SDT		0x011			// SDT

#define TS_PACKET_SIZE		188

#endif	// #ifndef