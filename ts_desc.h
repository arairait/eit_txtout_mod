#ifndef _INC_TS_DESC_H_
#define _INC_TS_DESC_H_

#include "aribstr.h"

#define DSCTAG_SHORTEVT		0x4D
#define DSCTAG_EXTEVT		0x4E
#define DSCTAG_COMPONENT	0x50
#define DSCTAG_CONTENT		0x54
#define DSCTAG_SERVICE		0x48

// SI��BCD������tm�\���̂ɕϊ�
static void _convert_time_bcd(TIME_BCD time, struct tm &rt)
{
	rt.tm_hour	= ((time>>20)&0xF) * 10 + ((time>>16)&0xF);
	rt.tm_min	= ((time>>12)&0xF) * 10 + ((time>> 8)&0xF);
	rt.tm_sec	= ((time>> 4)&0xF) * 10 + ((time>> 0)&0xF);
}

// SI�̓��t�����`����tm�\���̂ɕϊ�
static void _convert_time_jst40(TIME_JST40 time, tm &t)
{
	int tnum = (time >> 24) & 0xFFFF;		// ���16�r�b�g���o��
  
	t.tm_year = static_cast<int>((tnum - 15078.2) / 365.25);
	t.tm_mon = static_cast<int>(((tnum - 14956.1) - (int)(t.tm_year * 365.25)) / 30.6001);
	t.tm_mday = (tnum - 14956) - (int)(t.tm_year * 365.25) - (int)(t.tm_mon * 30.6001);
	if ( t.tm_mon == 14 || t.tm_mon == 15 )
	{
		t.tm_year++;
		t.tm_mon -= 1 + 12;
	}
	else
		t.tm_mon--;

	_convert_time_bcd( static_cast<TIME_BCD>(time), t );

	// tm��month��0�`11�̊ԂłȂ��Ƒʖ�
	t.tm_mon--;
}

// SI�̓��t�����`����time_t�ɕϊ�
static time_t _convert_time_jst40(TIME_JST40 time)
{
	tm tm_b;

	_convert_time_jst40( time, tm_b );
	return mktime( &tm_b );
}

// �L�q�q�N���X
class CDescriptor
{
public:
	int			tag;						// �L�q�q�^�O
	VECT_UINT8	buf;						// �f�[�^
public:
	// Data���Z�b�g����B�}�C�i�X��Ԃ��ƃG���[
	// �v���X�̏ꍇ�A���̋L�q�q�{�̂̃T�C�Y��Ԃ�
	int			SetData(VECT_UINT8::const_iterator beg, VECT_UINT8::const_iterator end)
	{	
		VECT_UINT8::const_iterator data = beg;
		if ( end - beg < 2 )
			return -1;
		tag	= beg[0];
		int len = beg[1];			
		if ( beg+2+len > end )
			return -1;
		buf.clear();
		std::copy( beg+2, beg+2+len, std::back_inserter( buf ) );
		return len;					// length��Ԃ�
	}
};

// �h���L�q�q�N���X
// �Z�`���C�x���g�L�q�q
class CShortEventDesc : public CDescriptor
{
public:
	tstring		name_str;
	tstring		desc_str;
public:
	CShortEventDesc();
	CShortEventDesc(const CDescriptor &desc)
	{
		const UINT8	*pp = &desc.buf[0];	
		const UINT8	*ep = pp + desc.buf.size();
		int buf_len = desc.buf.size();
		int src_len;
		tag = desc.tag;
		if ( desc.tag != DSCTAG_SHORTEVT )
			throw CDefExcept(_T("<CShortEventDesc>�L�q�q�^�O��%X�łȂ�%X�ł�"), DSCTAG_SHORTEVT, desc.tag );
		if ( buf_len < 4 )
			throw CDefExcept(_T("<CShortEventDesc>�L�q�q�o�C�g�������Ȃ�(%d)"), buf_len );
		pp += 3;			// �ŏ���3�o�C�g�͖���("jpn"�Ɠ����Ă�͂�)
		src_len = *pp++;
		if ( pp + src_len >= ep )		// ����desc_str��len�܂Ń`�F�b�N
			throw CDefExcept(_T("<CShortEventDesc>�L�q�q�o�C�g�������Ȃ�(%d)"), buf_len );
		AribToString( &name_str, pp, src_len );
		
		pp += src_len;
		src_len = *pp++;
		if ( pp + src_len > ep )
			throw CDefExcept(_T("<CShortEventDesc>�L�q�q�o�C�g�������Ȃ�(%d)"), buf_len );
		AribToString( &desc_str, pp, src_len );
	}
};

// �R���e���g�L�q�q
class CContentDesc : public CDescriptor
{
public:
	int		content_nibble_level_1;
	int		content_nibble_level_2;
	int		user_nibble_1;
	int		user_nibble_2;
public:
	CContentDesc(const CDescriptor &desc)
	{
		tag = desc.tag;
		if ( desc.tag != DSCTAG_CONTENT )
			throw CDefExcept(_T("<CContentDesc>�L�q�q�^�O��0x%X�łȂ�0x%X�ł�"), DSCTAG_CONTENT, desc.tag );
		if ( desc.buf.size() < 2 )
			throw CDefExcept(_T("<CContentDesc>�L�q�q�o�C�g����%d��菭�Ȃ�(%d)"), 2, desc.buf.size() );
		const UINT8	*sp = &desc.buf[0];	
		int size = desc.buf.size();
		while ( TRUE ) {
			content_nibble_level_1 = (sp[0] >> 4) & 0xF;
			content_nibble_level_2 = (sp[0] >> 0) & 0xF;
			user_nibble_1 = (sp[1] >> 4) & 0xF;
			user_nibble_2 = (sp[1] >> 0) & 0xF;
			sp += 2 ;
			size -= 2 ;
			// �W������=�g���̏ꍇ�͎�������
			if ( size >= 2 && content_nibble_level_1 == 0xe ) continue ;
			break ;
		}
	}
};

// �R���|�[�l���g�L�q�q
class CComponentDesc : public CDescriptor
{
public:
	int		stream_content;
	int		component_type;
	int		component_tag;
	tstring	text;
public:
	CComponentDesc(const CDescriptor &desc)
	{
		const UINT8	*sp = &desc.buf[0];	
		int buf_len = desc.buf.size();
		tag = desc.tag;
		if ( desc.tag != DSCTAG_COMPONENT )
			throw CDefExcept(_T("<CComponentDesc>�L�q�q�^�O��0x%X�łȂ�0x%X�ł�"), DSCTAG_COMPONENT, desc.tag );
		if ( buf_len < 6 )
			throw CDefExcept(_T("<CComponentDesc>�L�q�q�o�C�g�������Ȃ�(%d)"), buf_len );
		stream_content = sp[0] & 0xF;
		component_type = sp[1];
		component_tag = sp[2];
		sp += 6;
		AribToString( &text, sp, buf_len - 6 );
	}
};

class CServiceDesc : public CDescriptor
{
public:
	int		service_type;
	tstring	service_provider_name;
	tstring	service_name;
public:
	CServiceDesc(const CDescriptor &desc)
	{
		const UINT8 *sp = &desc.buf[0];
		const UINT8 *ep = sp + desc.buf.size();		// �I���`�F�b�N�p
		int buf_len = desc.buf.size();
		tag = desc.tag;
		if ( desc.tag != DSCTAG_SERVICE )
			throw CDefExcept(_T("<CServiceDesc>�L�q�q�^�O��0x%X�łȂ�0x%X�ł�"), DSCTAG_SERVICE, desc.tag );
		int str_len;
		if ( buf_len < 2 )
			throw CDefExcept(_T("<CServiceDesc>�L�q�q�o�C�g�������Ȃ�(%d)"), buf_len );
		service_type = sp[0];
		str_len = sp[1];
		sp += 2;
		if ( sp + str_len >= ep )					// service_name_len�̏��܂Ń`�F�b�N
			throw CDefExcept(_T("<CServiceDesc>�L�q�q�o�C�g�������Ȃ�(%d)"), buf_len );
		AribToString( &service_provider_name, sp, str_len );
		sp += str_len;
		str_len = *sp++;
		if ( sp + str_len > ep )
			throw CDefExcept(_T("<CServiceDesc>�L�q�q�o�C�g�������Ȃ�(%d)"), buf_len );
		AribToString( &service_name, sp, str_len );
	}
};

// �g���`���C�x���g�L�q�q
class CExtEventDesc : public CDescriptor
{
public:
	class CItem
	{
	public:
		tstring		description;	// ���ږ�
		VECT_UINT8	text;			// �����B�ʂ̊g���`���C�x���g�L�q�q�ƂȂ����Ă鎖������̂ŁA�o�C�g�z��(��łȂ���)
	};
	typedef std::vector<CItem>	VECT_ITEM;
public:
	int			descriptor_number;
	int			last_descriptor_number;
	VECT_UINT8	text;				// ���ڂȂ������B�ʂ̊g���`���C�x���g�L�q�q�ƂȂ����Ă鎖������̂ŁA�o�C�g�z��(��łȂ���)
	VECT_ITEM	items;
public:
	CExtEventDesc(const CDescriptor &desc)
	{
//		VECT_UINT8::const_iterator sp = desc.buf.begin();
		VECT_UINT8::const_iterator pp = desc.buf.begin();
		VECT_UINT8::const_iterator ep = desc.buf.end();

		int buf_len = desc.buf.size();
		int len_of_items;
		int str_len;
		CItem item;

		tag = desc.tag;
		if ( desc.tag != DSCTAG_EXTEVT )
			throw CDefExcept(_T("<CExtEventDesc>�L�q�q�^�O��0x%X�łȂ�0x%X�ł�"), DSCTAG_EXTEVT, desc.tag );
		if ( buf_len < 5 )		// length_of_items�܂ł̃`�F�b�N
			throw CDefExcept(_T("<CExtEventDesc>�L�q�q�o�C�g�������Ȃ�(%d)"), buf_len );
		descriptor_number = (pp[0] >> 4) & 0xF;
		last_descriptor_number = (pp[0] >> 0) & 0xF;
		pp += 4;				// +3��language_code("jpn")
		len_of_items = *pp++;
		while ( len_of_items > 0 )
		{
			str_len = *pp++;
			if ( pp + str_len >= ep )				// ����len�܂�
				throw CDefExcept(_T("<CExtEventDesc>�L�q�q�o�C�g�������Ȃ�(%d)"), buf_len );
			AribToString( &item.description, &pp[0], str_len );
			len_of_items -= str_len + 1;
			pp += str_len;

			str_len = *pp++;
			if ( pp + str_len > ep )
				throw CDefExcept(_T("<CExtEventDesc>�L�q�q�o�C�g�������Ȃ�(%d)"), buf_len );
			std::copy( pp, pp+str_len, std::back_inserter(item.text) );
			len_of_items -= str_len + 1;
			pp += str_len;

			items.push_back( item );
		}
		if ( len_of_items )
			DBG_LOG_ASSERT( _T("\rlen_of_items��0�ł͂Ȃ�:%d\n"), len_of_items );
		str_len = *pp++;
		if ( pp + str_len > ep )
			throw CDefExcept(_T("<CExtEventDesc>�L�q�q�o�C�g�������Ȃ�(%d)"), buf_len );
		std::copy( pp, pp+str_len, std::back_inserter(text) );
	}
	
};

typedef std::vector<CDescriptor>	VECT_DESC;

#endif