//-------------------------------------------------
// %1�F��������ts�t�@�C��
// %2�F�W��������͂���T�[�r�X�ԍ��B�P�T�[�r�X���������ĂȂ��Ȃ�w�肷��K�v�Ȃ�
//-------------------------------------------------
//������ �K�v�ɉ����ĕύX !!!!!
var prm_strEITOutPath = "E:\\Tools\\PT1\\script\\eit_txtout.exe";	// eit_txtout�̃p�X
var prm_strEITOutPrm = "-pso10% -r10s"
var prm_isEITOutLog = true;
var prm_strTsStorePath = "c:\\share\\ts";
var prm_isMove = false;												// �R�s�[/�ړ�
var prm_CopyCmd = "";												// �R�s�[�R�}���h�͊O���R�}���h���g�p�\
//var prm_CopyCmd = "E:\\Tools\\FireCopy\\FFC.exe \"<src>\" /min /t /to:\"<dstdir>\"";
																	// �R�s�[�R�}���h�B�ȉ��̕�����͒u�������
																	// <src>	:�R�s�[���t�@�C����
																	// <dst>	:�R�s�[��t�@�C����
																	// <dstdir>	:�R�s�[��t�H���_��
//������ �K�v�ɉ����ĕύX !!!!!
//*** �͂��߂� ***
// ���̃X�N���v�g��store_dir���̃t�H���_�\����ύX���܂���B
// �܂�A�����I�ɃW����������^�C�g�����̓������t�H���_���쐬����B�Ƃ������Ƃ͂��܂���B
// ������t�H���_���ōł������ɍ��v����T�u�t�H���_���Ɉړ�/�R�s�[���܂��B
//
// �ړ�/�R�s�[����t�H���_���Ɋ��ɓ����̃t�@�C�������݂����ꍇ�A���ړ�/�R�s�[���悤�Ƃ��Ă���t�@
// �C���^�C�g���̖�����(����)��t�����Ĉړ�/�R�s�[���܂�(�A���R�s�[���ƃR�s�[�悪�����t�H���_�̏�
// ���͉������܂���)
//
// �R�s�[/�ړ��ǂ�����s������prm_isMove�̒l�Ɉ˂�܂�
//
//*** ����d�l ***
//�ȉ��̃t�H���_�\���Ƃ��܂�
//	<store_dir>
//		<@�f��>
//		<@�A�j��>
//			<���p���O��>
//			<���p��>
//			<fate�^zero>
//			<�܊�̃V���iIII>
//
// (1) �n���ꂽ�t�@�C����(src_fpath)���ɃT�u�t�H���_�����܂܂��ꍇ�A���̃t�H���_�Ɉړ�/�R�s�[���܂�
// 	   �����̃t�H���_���Ɋ܂܂��ꍇ�A�Œ���v����t�H���_���ƂȂ�܂�
// 	   (��) ���p���O�� �J���I�X�g���̏�.ts �� /store_dir/@�A�j��/���p���O��
// 	   			�� <���p��>(3����)���<���p���O��>(5����)�̕���������v���Ă�̂ł����炪�I�������
//
// (2) �t�@�C����(�^�C�g��)���܂ރt�H���_���Ȃ������ꍇ�Aeit_txtout�œǂݍ��񂾃W������(g_GenreStr
// 	   �̒l)�ƈ�v����t�H���_�ɃR�s�[�^�ړ����܂�	
// 	   (��) �_�C�n�[�h�S.ts �� /store_dir/@�f��	(�_�C�n�[�h�S.ts�̃W��������"�f��"�ł���ꍇ)
//
// (3) �^�C�g���ɂ��W�������ɂ����v���Ȃ������ꍇ�A<store_dir>�����ɃR�s�[�^�ړ����܂��B
// 	   (��) ��ɑ嗤 ��1�b.ts �� /store_dir	
//
// ���̑�
// (a) �ԑg���͕����ǂɂ���āA�S�p/���p �啶��/�������A�󔒂̗L�����o���o���Ȃ̂őS���������m�Ƃ���
// 	   �����܂��B�܂����[�}����(�T,�U,�V)�ƃ��[�}�������A���t�@�x�b�g(I,II,III)���������m�ƈ����܂�
// 	   (��)	Fate�^Zero #01.ts				�� store_dir\@�A�j��\fate�^zero
// 	   		�e�������^�y������ ��1�b.ts		�� store_dir\@�A�j��\fate�^zero
// 	   		�܊�̃V���i �V�@#01.ts			�� store_dir\@�A�j��\�܊�̃V���iIII
// 	   		�܊�̃V���i�V�@#01.ts			�� store_dir\@�A�j��\�܊�̃V���iIII
// 	   		�܊�̃V���iIII #01.ts			�� store_dir\@�A�j��\�܊�̃V���iIII
// 	   �Ȃ�������(���O)�A�A���r�A����(�P�Q�R)�A���[�}����(�T�U�V)�͓������m�Ƃ��Ă͈���Ȃ��̂�
// 	   (��) ���p���R�� �J���I�X�g���̏�.ts �� store_dir\@�A�j��\���p��
// 	        ���p���V�� �J���I�X�g���̏�.ts �� store_dir\@�A�j��\���p��
// 	   �ƂȂ�܂��B
// 	   ��L�̂��Ƃ���A�T�u�t�H���_�ɑS�p/���p�A�󔒂̗L���̈قȂ�t�H���_�����݂��Ă�Ǝv�����ʂ��
// 	   �����Ȃ���������܂���B
// 	   �܂�<fate�^zero>�t�H���_��<�e�������^�y������>�t�H���_��store_dir�̃T�u�t�H���_���ɂ���Ƃ�
// 	   ����ɓ��邩��FileSystemObject�̎d�l�ƃT�u�t�H���_�̍\���Ɉ˂�܂��B��ԍŏ��Ɉ�v�����t�H���_
// 	   �ɂȂ�Ǝv���܂����A���������Ĉ�ԍŏ��ƒ�`���邩�̐����͏ȗ����܂��B
//
// (b) store_dir���̃T�u�t�H���_��OS�����e����΁A�ǂꂾ���[���K�w�ɍ���Ă��S�Č������锤�ł��B
//
// (c) �W���������t�H���_��ύX�������ꍇ�Ag_GenreStr�ɃW���������t�H���_�������Ă�ꏊ���C�����ĉ�����
//     �A���A�A�j���W��������"�A�j��"�Ƃ����W���������t�H���_�Ƃ��ɂ���ƁA�u�A�j���[�^�̂��d���v�݂�����
//     �h�L�������^���[�ԑg�������������ꍇ�Ƀt�@�C����(�^�C�g��)��v����̂ŁA���܂�ԑg���Ŏg���Ȃ���
//     ���ȃt�H���_���ɂ��Ă��������������ł��B
//     
// (d) eit_txtout�̏o�̓t�@�C��output.txt���c���ğT�������ꍇ��
// 			g_objFileSys.DeleteFile( output_file );
// 	   �̃R�����g���O���Ƃ����B
//**************************************************
var g_GenreStr;														// �ǂݍ���TS�t�@�C���̃W������
var g_objFileSys = new ActiveXObject("Scripting.FileSystemObject");
var g_objShell = WScript.CreateObject("WScript.Shell");

//**************************************************
// �R�s�[
// �y�����z
// 	src_file		�R�s�[���t�@�C��
// 	dst_file		�R�s�[��t�@�C��
// 	is_over_write	�㏑������H
// 	is_move			�ړ�(�R�s�[��R�s�[���t�@�C�����폜)����?
//
//**************************************************
function copy_file(src_file, dst_file, is_over_write, is_move)
{
	var date_1, date_2;
	var drv_src = g_objFileSys.GetDriveName(src_file);
	var drv_dst = g_objFileSys.GetDriveName(dst_file);
	// �R�s�[/�ړ�����K�v�����邩�ǂ����𔻕ʁB�����ꏊ�ɂ���t�@�C���͏������Ȃ�
	if ( g_objFileSys.FolderExists( dst_file ) )					// �R�s�[�悪�t�H���_�̏ꍇ
	{
		if ( g_objFileSys.GetParentFolderName(src_file).toLowerCase() == dst_file.toLowerCase() )
			return;
	}
	else if ( src_file.toLowerCase() == dst_file.toLowerCase() )	// �R�s�[�悪�t�@�C���̏ꍇ
		return;

	if ( is_move )
		WScript.Echo( "Move: " + src_file +  " -> " + dst_file );
	else
		WScript.Echo( "Copy: " + src_file +  " -> " + dst_file );
	date_1 = new Date();
	if ( !is_over_write && is_move && drv_src == drv_dst )
		g_objFileSys.MoveFile( src_file, dst_file );
	else
	{
		if ( prm_CopyCmd == null || prm_CopyCmd == "" )
		{
			// ��`����ĂȂ��Ƃ��A���ʂ̃R�s�[���g�p����
			g_objFileSys.CopyFile( src_file, dst_file, is_over_write );
		}
		else
		{
			var is_rename = false;
			var org_src_file = "";
			var s_cmd_line = prm_CopyCmd;
			s_cmd_line = s_cmd_line.replace("<dst>", dst_file );
			if ( s_cmd_line.indexOf( "<dstdir>" ) >= 0 )
			{
				if ( g_objFileSys.FolderExists( dst_file ) )
					s_cmd_line = s_cmd_line.replace("<dstdir>", dst_file );
				else
				{
					s_cmd_line = s_cmd_line.replace("<dstdir>", g_objFileSys.GetParentFolderName(dst_file) );
					// src_file ��K���Ȗ��O�ɕύX(��Ń��l�[������)
					// �R�s�[���ƃR�s�[�悪�����ꍇ�A���L��
					var d = new Date();
					var new_src_file = g_objFileSys.GetParentFolderName(src_file);
					new_src_file = g_objFileSys.BuildPath( new_src_file, g_objFileSys.GetTempName() );
					g_objFileSys.MoveFile( src_file, new_src_file );
					org_src_file = src_file;
					src_file = new_src_file;
					is_rename = true;
				}
			}
			s_cmd_line = s_cmd_line.replace("<src>", src_file );
			g_objShell.Run( s_cmd_line, 1, true );
			if ( is_rename )
			{	// ���O��ύX����
				var src_fname = g_objFileSys.GetFileName( src_file );
				var dst_fname = g_objFileSys.GetFileName( dst_file );
				if ( src_fname != dst_fname )
				{
					var dst_dir = g_objFileSys.GetParentFolderName( dst_file );
					g_objFileSys.MoveFile( 
							g_objFileSys.BuildPath(dst_dir, src_fname),
							g_objFileSys.BuildPath(dst_dir, dst_fname)
					);
				}
				// ���t�@�C���̖��O�����ɖ߂�
				if ( !is_move )
					g_objFileSys.MoveFile( src_file, org_src_file );
			}
		}
		// �ړ��̎��͌��t�@�C�����폜����
		if ( is_move )
			g_objFileSys.DeleteFile( src_file );

	}
	date_2 = new Date();
	WScript.Echo( (date_2.valueOf() - date_1.valueOf())/1000 + "sec" );
}

//**************************************************
// eit_txtout �t�@�C������
// �y�����z
//	strTsFile		����TS�t�@�C��
//	svc_no			�L�[�ƂȂ�T�[�r�X��
//	strOutDir		output.txt���i�[����t�H���_
//	
//	�y�Ԃ�l�z
//		-1		���s
//		��		bit:0	inc_svc
//				bit:1	multi_svc
//				bit:2	bs_nan
//**************************************************
function eit_txtout_procs( strTsFile, strSvsNo, strOutDir )
{
	var cmd_line = prm_strEITOutPath + " " + prm_strEITOutPrm;
	if ( prm_isEITOutLog )
	{
		cmd_line += " -l\"" + g_objFileSys.BuildPath( g_objFileSys.GetParentFolderName(strTsFile), "eit_txtout_log.txt" ) + "\"";
	}
	WScript.Echo("*** eit_txtout_procs ***");
	cmd_line += " \"" + strTsFile + "\"";
	var output_file = g_objFileSys.BuildPath( strOutDir, "output.txt" );
	cmd_line += " \"" +  output_file + "\"";
	// �O��output.txt���폜�B���̂܂܂ɂ��Ă����Ă����ʏ㏑������邪�A�N�����s�����ۂɑO�̂��c���Ă�ƍ������邩������Ȃ��̂ŁB
	if ( g_objFileSys.FileExists( output_file ) )
		g_objFileSys.DeleteFile( output_file );
	WScript.Echo( cmd_line );
	if ( g_objShell.Run( cmd_line, 1, true ) == 0 ) 
    {
        var fs = g_objFileSys.OpenTextFile(output_file, 1);
		var lines = fs.ReadAll();
		var ret = 0;
		fs.Close();
		// �W������������擾
		var s_find_ptn;				// �����p�^�[��
		if ( strSvsNo == null || strSvsNo == "" )
			// �T�[�r�X�w��Ȃ��̏ꍇ�A�ŏ���EVT����������B�W�������l���L��
			s_find_ptn = "^EVT\\t\\d+";
		else
		
			// EVT	<strSvsNo>����ƂȂ�ŏ���EVT����������B�W�������l���L��
			s_find_ptn = "^EVT\\t"+strSvsNo;
		s_find_ptn += "\\t\\d+\\t[\\d :/]+\\t[\\d :/]+\\t[^\\t]+\\t([\\-\\d]+)\\t([^\\t]+)\\t.*$";
		if ( lines.match( new RegExp(s_find_ptn, "m") ) != null )
		{
			var genre_val = RegExp.$1.valueOf();
			//WScript.Echo( "�W�������啪��" + RegExp.$2 );
			if ( genre_val >= 0 )
			{
				switch ( (genre_val >> 4) & 0xF )
				{
				case 0:	g_GenreStr = "@�j���[�X";			break;
				case 1: 
					if ( (genre_val & 0xF) == 0 )	g_GenreStr = "@�j���[�X";		// �X�|�[�c�j���[�X�̓j���[�X�ɓ����
					else							g_GenreStr = "@�X�|�[�c";
					break;
				case 2:	g_GenreStr = "@���C�h�V���[";		break;
				case 3: g_GenreStr = "@�h���}";				break;
				case 4: g_GenreStr = "@���y";				break;
				case 5: g_GenreStr = "@�o���G�e�B";			break;
				case 6: 
					if ( (genre_val & 0xF) == 2 )	g_GenreStr = "@�A�j��";			// �A�j���f��̓A�j���ɓ����
					else							g_GenreStr = "@�f��";
					break;
				case 7: g_GenreStr = "@�A�j��";				break;
				case 8: g_GenreStr = "@�h�L�������^���[";	break;
				case 9: g_GenreStr = "@����";				break;
				case 10: 
					if ( (genre_val & 0xF) <= 6 )	g_GenreStr = "@�";
					else							g_GenreStr = "@����";
					break;
				case 11: g_GenreStr = "@����";				break;
				}
				g_GenreStr = convert_zenhan( g_GenreStr );
				g_GenreStr = g_GenreStr.toLowerCase();
			}
		}
		else
			g_GenreStr = "";
		WScript.Echo( "�W�������F" + g_GenreStr );
		// output.txt���c���ĂğT�������ꍇ�̓R�����g�A�E�g
//		g_objFileSys.DeleteFile( output_file );

		return ret;
	}
	else
		return -1;
}

//**************************************************
// TS�t�@�C�����������l�[�����Ĉړ�/�R�s�[
// 	
// �y�����z
// 	src_file		�ړ����t�@�C�����t���p�X
// 	s_dst_dir		�ړ���t�H���_
// 	�y�Ԃ�l�z
// 	�ړ���̃t�@�C�����t���p�X
//**************************************************
function copy_file_auto_rename(src_file, s_dst_dir)
{
	var aft_name;
	// �ړ���ɓ������O�̃t�@�C�����������ꍇ�A(?)��t����
	aft_name = g_objFileSys.GetFileName( src_file );
	aft_name = g_objFileSys.BuildPath( s_dst_dir, aft_name );
	if ( g_objFileSys.FileExists( aft_name ) )
	{
		var s_base = g_objFileSys.GetBaseName(src_file);
		var s_ext = g_objFileSys.GetExtensionName(src_file);
		var i = 0;
		do
		{
			i++;
			aft_name = g_objFileSys.BuildPath(s_dst_dir, s_base+"("+i+")."+s_ext );
		}while ( g_objFileSys.FileExists( aft_name ) );
	}
	// �R�s�[�^�ړ�
	copy_file( src_file, aft_name, false, prm_isMove );
	return aft_name;
}

//**************************************************
// �S�p�����p�ϊ��B�A���J�i�͕ϊ����Ȃ�
// 	
// �y�����z
// 	src_fpath	
//**************************************************
// JScript�͒萔���Ȃ��̂ŁA�d���Ȃ��ɂ����Ő錾(�����������̂͂Ȃ񂩌��Ȃ̂�)
var c_full_alpha = "�`�a�b�c�d�e�f�g�h�i�j�k�l�m�n�o�p�q�r�s�t�u�v�w�x�y����������������������������������������������������";
var c_half_alpha = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
var c_full_number = "�O�P�Q�R�S�T�U�V�W�X";
var c_half_number = "0123456789";
var c_full_symbol = "�@�I�h���������f�V�i�j���{�C�[�|�D�^�F�G�������H���m���n�O�Q�M�o�b�p�`";
var c_half_symbol = " !\"#$%&''()*+,--./:;<=>?@[\\]^_`{|}~";
var c_full_romanum = "�T�U�V�W�X�Y�Z�[�\�]";
var c_half_romanum = ("I,II,III,IV,V,VI,VII,VIII,IX,X").split(",");		// 1�����Œu���������Ȃ��̂ŁA�R���}�ŋ�؂�
function convert_zenhan( str )
{
	var is_full_alpha=false, is_full_number=false, is_full_symbol=false, is_full_romanum=false;
	var i;
	if ( str == null )
		return null;
	// �ϊ�����S�p�������܂܂�邩����
	for ( i = 0; i < str.length; i++ )
	{
		var code = str.charCodeAt(i);
		if ( !is_full_symbol
			&& (code == 0x201d || code == 0x2019 || code == 0x3000		// �h�f�@
				||	code == 0x30fc || code == 0xff01					// �[�I
				||	(0xff03<= code && code <= 0xff0f)					// ���������V�i�j���{�C�D�|�D
				||	(0xff1a<= code && code <= 0xff20)					// �F�G�������H��
				||	(0xff3b<= code && code <= 0xff40 && code != 0xff3c)	// �m�n�O�Q�M	�A���_�͏���
				||	(0xff5b <= code && code <= 0xff5e)					// �o�b�p�`
				||	code == 0xff5e)										// ��
			)
			is_full_symbol = true;
		else if (!is_full_number && 0xff10 <= code && code <= 0xff19)
			is_full_number = true;
		else if (!is_full_alpha && ((0xff21 <= code && code <= 0xff3a) || (0xff41 <= code && code <= 0xff5a)))
			is_full_alpha = true;
		else if (!is_full_romanum && 0x2160 <= code && code <= 0x2169)
			is_full_romanum = true;
	}
	var s1, s2;
	var sret = str;
	if ( is_full_alpha )
	{
		for ( i = 0; i < c_full_alpha.length; i++ )
		{
			s1 = c_full_alpha.substr(i,1);
			s2 = c_half_alpha.substr(i,1);
			sret = sret.replace(s1, s2);
		}
	}
	if ( is_full_number )
	{
		for ( i = 0; i < c_full_number.length; i++ )
		{
			s1 = c_full_number.substr(i,1);
			s2 = c_half_number.substr(i,1);
			sret = sret.replace(s1, s2);
		}
	}
	if ( is_full_symbol )
	{
		for ( i = 0; i < c_full_symbol.length; i++ )
		{
			s1 = c_full_symbol.substr(i,1);
			s2 = c_half_symbol.substr(i,1);
			sret = sret.replace(s1, s2);
		}
	}
	if ( is_full_romanum )
	{
		for ( i = 0; i < c_full_romanum.length; i++ )
		{
			s1 = c_full_romanum.substr(i,1);
			s2 = c_half_romanum[i];
			sret = sret.replace(s1, s2);
		}
	}
	return sret;
}

//**************************************************
// �������r�̂��߂̕ϊ�
// 	
// �y�����z
// 	str				������
//
// �y�Ԃ�l�z
// �ϊ����ꂽ������
// 	�S�p�^���p�A�啶���^�������A�󔒂̗L��
// 	��S�ē��ꎋ���邽�ߕϊ�����
//**************************************************
function convert_str( str )
{
	var s_ret = str;

	s_ret	= convert_zenhan( s_ret );			// �S�p�����p
	s_ret	= s_ret.toLowerCase( s_ret );		// �S����������
	s_ret	= s_ret.replace( " ", "");			// �󔒂𖳎�

	return s_ret;
}

//**************************************************
// 
// 	
// �y�����z
// 	s_parent_dir	�e�t�H���_
// 	str				������(�ϊ��ς�)
// �y�Ԃ�l�z
// str���܂܂�Ă���܂��̓W������������(g_GenreStr)�ƈ�v����T�u�t�H���_���������ꍇ�A���̃p�X��Ԃ��B
//
// �A��JScript�͈����̎Q�Ɠn�����o���Ȃ��̂ŁAstr���܂܂�Ă�̂��W������������ƈ�v�����̂����ʂ�����
// ���𔻕ʂ��邽�߁Astr���܂܂�Ă�ꍇ�ɂ�"<<"��擪�ɕt�����ĕԂ��B
//**************************************************
function find_includestr_dir( s_parent_dir, str )
{
	var objFolder = g_objFileSys.GetFolder( s_parent_dir );
	var fc = new Enumerator(objFolder.SubFolders);
	var ret = null;

	for ( ; !fc.atEnd(); fc.moveNext() )
	{
		var a_ret = find_includestr_dir( fc.item().Path, str );
		var ret_name;
		if ( a_ret != null )
		{
			if 		( ret == null )
				ret = a_ret;
			else if ( a_ret.length > 2 && a_ret.substr(0,2) == "<<" )
			{
				// �Œ���v�̂��߁A�t�H���_���̂����o���Ĕ�r
				var a_ret_name;
				if ( ret != null )		ret_name = g_objFileSys.GetFileName(ret);
				else 					ret_name = "";
				if ( a_ret != null )	a_ret_name = g_objFileSys.GetFileName(a_ret);
				else 					a_ret_name = "";

				if ( a_ret_name.length > ret_name.length )
					ret = a_ret;
			}
		}
		var s_name = convert_str( fc.item().Name );
		if 		( str.indexOf(s_name) >= 0 )
		{
			if ( ret == null || ret.length < 2 || ret.substr(0,2) != "<<" )
				ret = "<<" + fc.item().Path;						// �W���������t�H���_�Ƌ�ʂ��邽��"<<"��t����
			else
			{
				// �Œ���v�̂��߁A�t�H���_���̂����o���Ĕ�r
				if ( ret != null )	ret_name = g_objFileSys.GetFileName(ret);
				else 				ret_name = "";
				if ( s_name.length > ret_name.length )
					ret = "<<" + fc.item().Path;					// �W���������t�H���_�Ƌ�ʂ��邽��"<<"��t����
			}
		}
		else if ( ret == null && s_name == g_GenreStr )			// �W���������t�H���_�̏ꍇ�A
			ret = fc.item().Path;
	}
	return ret;
}

//**************************************************
// �ۑ��t�H���_(prm_strTsStorePath)�Ɉړ�(�܂��̓R�s�[)
// 	
// �y�����z
// 	src_fpath	�ړ�/�R�s�[����t�@�C��
//
//**************************************************
function tsfile_storedir_copy( src_fpath )
{
	var cvt_src_fbase = convert_str( g_objFileSys.GetBaseName(src_fpath) );
	var dst_path;

	dst_path = find_includestr_dir( prm_strTsStorePath, cvt_src_fbase );
	if ( dst_path != null && dst_path.length > 2 && dst_path.substr(0,2) == "<<" )
		dst_path = dst_path.substr(2);		// << ������
	if ( dst_path == null )
		dst_path = prm_strTsStorePath;
	if ( g_objFileSys.GetParentFolderName(src_fpath).toLowerCase() != dst_path.toLowerCase() )
		copy_file_auto_rename( src_fpath, dst_path );
}

function main_proc(s_tsfile, s_svcno)
{
	var ret;
	if ( !g_objFileSys.FileExists( s_tsfile ) )
	{
		WScript.Echo( "�t�@�C�������݂��܂���" + s_tsfile );
		return -1;
	}
	else if ( g_objFileSys.GetExtensionName( s_tsfile ).toLowerCase() != "ts" )
	{
		WScript.Echo( "TS�t�@�C�����w�肵�ĉ�����" + s_tsfile );
		return -2;
	}
	eit_txtout_procs( s_tsfile, s_svcno, g_objFileSys.GetParentFolderName(s_tsfile) );
	tsfile_storedir_copy( s_tsfile );
	return;
}

// global proc
//--------------------------------------------------
var strOrgTsFile;								// %1
var strSvsNo;									// %2
{
	strOrgTsFile = WScript.Arguments(0); 
	if ( WScript.Arguments.Length >= 2 )
		strSvsNo = WScript.Arguments(1);
	var date_1 = new Date();
	WScript.Echo("<<<<<< start "+date_1.toLocaleString() + " <<<<<<" );
	WScript.Echo("SVC:" + strSvsNo + " FILE:" + strOrgTsFile );
	main_proc(strOrgTsFile, strSvsNo);
	date_1 = new Date();
	WScript.Echo(">>>>>> end   "+date_1.toLocaleString() + " >>>>>>" );
}


