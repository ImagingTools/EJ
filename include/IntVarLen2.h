#ifndef INTVARLEN_H
#define INTVARLEN_H
#include <QtGui>



class IntVarLen2
{
public:
	IntVarLen2(quint32 &v) :
		value(v)
	{}

	friend QDataStream& operator>>(QDataStream &is, IntVarLen2 &v)
	{
		v.value = 0;
		quint8 c;
		do {
			is >> c;
			v.value = (v.value << 7) | (c & 0x7f);
		}while(c & 0x80);

		return is;
	}

	friend QDataStream& operator<<(QDataStream &os, IntVarLen2 &v)
	{
		quint32 value = v.value;

		quint64 buf = value & 0x7f;
		value = value >> 7;

		while(value) {
			buf = (buf << 8) | quint8((value & 0x7f) | 0x80);
			value = value >> 7;
		};

		while(buf & 0x80) {
			os << quint8(buf & 0xff);
			buf = buf >> 8;
		};
		os << quint8(buf & 0xff);

		return os;
	}
	int getValue() { return value; }

private:
	quint32 &value;
};



// class IntVarLen2
// {
// public:
//     IntVarLen2(){};
// //    IntVarLen(quint32 &v) :
// //        value(v)
// //    {}

// //    friend QDataStream& operator>>(QDataStream &is, IntVarLen &v)
// //    {
// //        v.value = 0;
// //        quint8 c;
// //        do {
// //            is >> c;
// //            v.value = (v.value << 7) | (c & 0x7f);
// //        }while(c & 0x80);

// //        return is;
// //    }



//     char * fromchararray(char *cp_array,quint32 &uint_res,quint32 &uint_len)
//     {
//         //quint32 value = 0;
//         //quint8 c;
//         uint_res = 0;
//         cp = cp_array;
//         //char *cpa;

//         do{
//             cpa = cp;
//             uint_res = (uint_res << 7) | (*cpa & 0x7f);
//             cp++;
//         }while(*cpa & 0x80);
//         uint_len = cp-cp_array;
//         return cp;
//     }
//     char*tochararray(quint32 uint,char *cp_array,quint32 &uint_len)
//     {
//         buf = uint & 0x7f;
//         uint = uint >> 7;

//         while(uint) {
//             buf = (buf << 8) | quint8((uint & 0x7f) | 0x80);
//             uint = uint >> 7;
//         };
//         cp = cp_array;
//         while(buf & 0x80)
//         {
//             *cp = (char)(buf & 0xff);
//             buf = buf >> 8;
//             cp++;
//         };
//         *cp = (char)(buf & 0xff);
//         cp++;
//         uint_len = cp-cp_array;
//         return cp;
//     }
// //    friend QDataStream& operator<<(QDataStream &os, IntVarLen &v)
// //    {
// //        quint32 value = v.value;

// //        quint64 buf = value & 0x7f;
// //        value = value >> 7;

// //        while(value) {
// //            buf = (buf << 8) | quint8((value & 0x7f) | 0x80);
// //            value = value >> 7;
// //        };

// //        while(buf & 0x80) {
// //            os << quint8(buf & 0xff);
// //            buf = buf >> 8;
// //        };
// //        os << quint8(buf & 0xff);

// //        return os;
// //    }


// //    void setValue(quint32 v)
// //    {
// //        this->value = v;
// //    }
// //    quint32 getValue() { return value; }

// private:
//     //quint32 &value;
//     quint64 buf;
//     char *cp;
//     char *cpa;
// };



//class MIntVarLen
//{
//public:

//    MIntVarLen()
//    {
//        //pds = new QDataStream(&ba,QIODevice::ReadWrite);
//        //IntVarLen ivl(uint32);
//        //pv = &ivl;
//        //cp  = new char[4];
//    };
//    ~MIntVarLen()
//    {
//        //delete pds;
//        //delete[] cp;
//    }

//    static char * tochararray(quint32 uint32,quint32 &uintlen,char*cp)
//    {
//        QByteArray ba;
//        IntVarLen v(uint32);
//        QDataStream ds(&ba,QIODevice::ReadWrite);
//        //this->uint32 = uint32;
//        ds<<v;
//        //IntVarLen ivl(uint32);
//        //*pds<<ivl;
//        uintlen = ba.length();
//        if(cp)
//            memcpy(cp,ba.data(),uintlen);
//        return cp;
//    }
////    quint32 fromchararray(char *cp)
////    {
////        ba.clear();
////        ba.setRawData(cp,4);
////        *pds>>*pv;
////        quint32 res = pv->getValue();
////        return res;
////    }
//    static quint32 make_pattern(char*cpterm,quint32 &ui_termlen,quint32 &uint_len1,
//                                char *cpinfo,quint32 &ui_infolen,quint32 &uint_len2,
//                                char*cppat)
//    {
//        //quint32 ui_shortuintlen1,ui_shortuintlen2;
//        tochararray(ui_termlen,uint_len1,cppat);
//        //memcpy(cppat,cp,ui_shortuintlen1);
//        memcpy(cppat+uint_len1,cpterm,ui_termlen);
//        tochararray(ui_infolen,uint_len2,cppat+uint_len1+ui_termlen);
//       // memcpy(cppat+ui_shortuintlen1+ui_termlen,cp,ui_shortuintlen2);
//        if(cpinfo)
//            memcpy(cppat+uint_len1+ui_termlen+uint_len2,cpinfo,ui_infolen);
//        return uint_len1+ui_termlen+uint_len2+ui_infolen;
//    }
//    static quint32 fromchararray(char *cp,quint32 &uint_len)
//    {
//        QByteArray ba;
//        ba.setRawData(cp,4);
//        //pds->
//        QDataStream ds(&ba,QIODevice::ReadWrite);
//        quint32 uint;
//        IntVarLen ivl(uint);
//        ds>>ivl;
//        quint32 res = ivl.getValue();
//        tochararray(res,uint_len,nullptr);
//        return res;
//    }
//    static quint32 analize_pattern(char *cp_pat,quint32 &uint_len1,quint32 &uint_termlen,quint32 &uint_len2,quint32 &uint_infolen)
//    {
//        uint_termlen = fromchararray(cp_pat,uint_len1);
//        uint_infolen = fromchararray(cp_pat+uint_len1+uint_termlen,uint_len2);
//        return uint_len1+uint_termlen+uint_len2+uint_infolen;
//    }
//    quint32 fromchararray(char *cp_array)
//    {
//        quint32 value = 0;
//        //quint8 c;
//        char*cp = cp_array;
//        do {

//            value = (value << 7) | (*cp & 0x7f);
//            cp++;
//        }while(*cp & 0x80);

//        return value;
//    }
//    int tochararray(quint32 uint,char *cp_array)
//    {
//        quint64 buf = uint & 0x7f;
//        uint = uint >> 7;

//        while(uint) {
//            buf = (buf << 8) | quint8((uint & 0x7f) | 0x80);
//            uint = uint >> 7;
//        };
//        char * cp = cp_array;
//        while(buf & 0x80)
//        {
//            *cp = (char)(buf & 0xff);
//            buf = buf >> 8;
//            cp++;
//        };
//        //os << quint8(buf & 0xff);
//        //uint_len = cp-cp_array;
//        return cp-cp_array;
//    }

//private:
////    QByteArray ba;
////    QDataStream *pds;
////    IntVarLen * pv;
////    quint32 uint32;
////    char* cp;
//    //quint32 ui_shortuintlen1;
//    //quint32 ui_shortuintlen2;
//};




#endif // INTVARLEN_H
