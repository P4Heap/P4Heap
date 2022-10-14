#include <core.p4>
#include <tna.p4>

/*************************************************************************
 ************* C O N S T A N T S    A N D   T Y P E S  *******************
*************************************************************************/
enum bit<16> ether_type_t {
    TPID       = 0x8100,
    IPV4       = 0x0800
}
enum bit<8>  ip_proto_t {
    ICMP  = 1,
    IGMP  = 2,
    TCP   = 6,
    UDP   = 17
}

type bit<48> mac_addr_t;

/*************************************************************************
 ***********************  H E A D E R S  *********************************
 *************************************************************************/
/*  Define all the headers the program will recognize             */
/*  The actual sets of headers processed by each gress can differ */

/* Standard ethernet header */
header ethernet_h {
    mac_addr_t    dst_addr;
    mac_addr_t    src_addr;
    ether_type_t  ether_type;
}

header ipv4_h{
	bit<4> 	version;
	bit<4> 	ihl;
	bit<8> 	diffserv;
    bit<16> total_len;
	bit<16> identification;
	bit<3> 	flags;
	bit<13> fragOffset;
	bit<8> 	ttl;
    bit<8> 	protocol;
	bit<16> checksum;
	bit<32> src_addr;
	bit<32> dst_addr;
}  
header MyFlow{
	bit<32> id;
}

struct my_ingress_headers_t{
	ethernet_h ethernet;
	ipv4_h ipv4;
	MyFlow myflow;
}

struct my_ingress_metadata_t {
	bit<32> id;
	bit<32> freq;
    bit<16> index2;
    bit<16> index3;
    bit<16> index4;
    bit<16> index5;
	bit<32> output1_1;
	bit<32> output1_2;
	bit<32> output2_1;
	bit<32> output2_2;
	bit<32> output3_1;
	bit<32> output3_2;
	bit<32> output4_1;
	bit<32> output4_2;
	bit<32> output5_1;
	bit<32> output5_2;
	bit<2> rng;
}

    /***********************  P A R S E R  **************************/

parser IngressParser(packet_in        pkt,
    /* User */
    out my_ingress_headers_t          hdr,
    out my_ingress_metadata_t         meta,
    /* Intrinsic */
    out ingress_intrinsic_metadata_t  ig_intr_md)
{
	state start{
		pkt.extract(ig_intr_md);
		pkt.advance(PORT_METADATA_SIZE);
        transition meta_init;
	}
    state meta_init {
        meta.freq = 1;
        meta.index2 = 0;
        meta.index3 = 0;
        meta.index4 = 0;
        meta.index5 = 0;
		meta.output1_1 = 0;
        meta.output1_2 = 0;
		meta.output2_1 = 0;
        meta.output2_2 = 0;
		meta.output3_1 = 0;
        meta.output3_2 = 0;
		meta.output4_1 = 0;
        meta.output4_2 = 0;
		meta.output5_1 = 0;
        meta.output5_2 = 0;
        meta.rng = 0;
        transition parse_ethernet;
    }


    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        /* 
         * The explicit cast allows us to use ternary matching on
         * serializable enum
         */        
        transition select((bit<16>)hdr.ethernet.ether_type) {
            (bit<16>)ether_type_t.IPV4            :  parse_ipv4;
            default :  accept;
        }
    }

	state parse_ipv4{
		pkt.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            (bit<8>)ip_proto_t.ICMP             : accept;
            (bit<8>)ip_proto_t.IGMP             : accept;
            (bit<8>)ip_proto_t.TCP              : parse_myflow;
            (bit<8>)ip_proto_t.UDP              : parse_myflow;
            default : accept;
        }
	}

	state parse_myflow{
		pkt.extract(hdr.myflow);
		meta.id = hdr.myflow.id;
		transition accept;
	}

}

struct data_elastic {
	bit<32> key;
	bit<32> vote;
};
struct data_lrfu {
	bit<32> key;
	bit<32> pure;
};
/* ingress */
control Ingress(/* User */
    inout my_ingress_headers_t                       hdr,
    inout my_ingress_metadata_t                      meta,
    /* Intrinsic */
    in    ingress_intrinsic_metadata_t               ig_intr_md,
    in    ingress_intrinsic_metadata_from_parser_t   ig_prsr_md,
    inout ingress_intrinsic_metadata_for_deparser_t  ig_dprsr_md,
    inout ingress_intrinsic_metadata_for_tm_t        ig_tm_md)
{
	CRCPolynomial<bit<32>>(0x04C11DB7,false,false,false,32w0xFFFFFFFF,32w0xFFFFFFFF) crc32a;
    CRCPolynomial<bit<32>>(0x741B8CD7,false,false,false,32w0xFFFFFFFF,32w0xFFFFFFFF) crc32b;
    CRCPolynomial<bit<32>>(0xDB710641,false,false,false,32w0xFFFFFFFF,32w0xFFFFFFFF) crc32c;
    CRCPolynomial<bit<32>>(0xAC240651,false,false,false,32w0xFFFFFFFF,32w0xFFFFFFFF) crc32d;
    CRCPolynomial<bit<32>>(0xAC240652,false,false,false,32w0xFFFFFFFF,32w0xFFFFFFFF) crc32e;
    CRCPolynomial<bit<32>>(0xAC240654,false,false,false,32w0xFFFFFFFF,32w0xFFFFFFFF) crc32g;
    CRCPolynomial<bit<32>>(0xAC240655,false,false,false,32w0xFFFFFFFF,32w0xFFFFFFFF) crc32h;
    CRCPolynomial<bit<32>>(0xAC240656,false,false,false,32w0xFFFFFFFF,32w0xFFFFFFFF) crc32i;

	Hash<bit<15> >(HashAlgorithm_t.CUSTOM,crc32a) hash_0;
	Hash<bit<16> >(HashAlgorithm_t.CUSTOM,crc32a) hash_1;
    Hash<bit<16> >(HashAlgorithm_t.CUSTOM,crc32b) hash_2;
    Hash<bit<16> >(HashAlgorithm_t.CUSTOM,crc32c) hash_3;
	Hash<bit<16> >(HashAlgorithm_t.CUSTOM,crc32d) hash_4;
    Hash<bit<16> >(HashAlgorithm_t.CUSTOM,crc32e) hash_5;
    Hash<bit<16> >(HashAlgorithm_t.CUSTOM,crc32g) hash_7;
    Hash<bit<16> >(HashAlgorithm_t.CUSTOM,crc32h) hash_8;
    Hash<bit<16> >(HashAlgorithm_t.CUSTOM,crc32i) hash_9;
	

	Register<data_elastic, bit<16> >(65536) elastic_stage1_1;
	RegisterAction<data_elastic, bit<16> , bit<32> >(elastic_stage1_1) elastic_stage1_1_insert=
	{
		void apply(inout data_elastic register_data,out bit<32> result)
		{
        if(register_data.vote==0 || register_data.key==meta.id)
        {
            result = register_data.key;
        }
        else
        {
            result = 0;
        }
		if(register_data.vote==0)
		{
			register_data.key = meta.id;
		}
		else
		{
			register_data.key = register_data.key;
		}

        if(register_data.vote!=0 && register_data.key!=meta.id)
		{
			register_data.vote = register_data.vote - 1;
		}
		else
		{
			register_data.vote = register_data.vote + 8;
		}
		
		}
	};
	Register<data_elastic, bit<16> >(32768) elastic_stage2_1;
	RegisterAction<data_elastic, bit<16> , bit<32> >(elastic_stage2_1) elastic_stage2_1_insert=
	{
		void apply(inout data_elastic register_data,out bit<32> result)
		{
        if(register_data.vote ==0 || register_data.key==meta.output1_1)
        {
            result = register_data.key;
        }
        else
        {
            result = 0;
        }
		
		if(register_data.vote==0)
		{
			register_data.key = meta.output1_1;
		}
		else
		{
			register_data.key = register_data.key;
		}

		if(register_data.vote!=0 && register_data.key!=meta.output1_1)
		{
			register_data.vote = register_data.vote - 1;
		}
		else
		{
			register_data.vote = register_data.vote + 8;
		}
		
		}
	};
	Register<data_elastic, bit<16> >(16384) elastic_stage3_1;
	RegisterAction<data_elastic, bit<16> , bit<32> >(elastic_stage3_1) elastic_stage3_1_insert=
	{
		void apply(inout data_elastic register_data,out bit<32> result)
		{
        if(register_data.vote ==0 || register_data.key==meta.output2_1)
        {
            result = register_data.key;
        }
        else
        {
            result = 0;
        }
		
		if(register_data.vote==0)
		{
			register_data.key = meta.output2_1;
		}
		else
		{
			register_data.key = register_data.key;
		}

		if(register_data.vote!=0 && register_data.key!=meta.output2_1)
		{
			register_data.vote = register_data.vote - 1;
		}
		else
		{
			register_data.vote = register_data.vote + 10;
		}
		
		}
	};
	Register<bit<32>, bit<16>  >(65536) elastic_stage1_2;
	RegisterAction<bit<32>, bit<16> , bit<32> >(elastic_stage1_2) elastic_stage1_2_add=
	{
		void apply(inout bit<32> register_data,out bit<32> result)
		{
		    result = 0;
			register_data = register_data + meta.freq;
		
		}
	};
    RegisterAction<bit<32>, bit<16> , bit<32> >(elastic_stage1_2) elastic_stage1_2_chan=
	{
		void apply(inout bit<32> register_data,out bit<32> result)
		{
		    result = register_data;
			register_data = meta.freq;
		}
	};
	Register<bit<32>, bit<16>  >(32768) elastic_stage2_2;
	RegisterAction<bit<32>, bit<16> , bit<32> >(elastic_stage2_2) elastic_stage2_2_add=
	{
		void apply(inout bit<32> register_data,out bit<32> result)
		{
		    result = 0;
			register_data = register_data + meta.output1_2;
		
		}
	};
    RegisterAction<bit<32>, bit<16> , bit<32> >(elastic_stage2_2) elastic_stage2_2_chan=
	{
		void apply(inout bit<32> register_data,out bit<32> result)
		{
		    result = register_data;
			register_data = meta.output1_2;
		}
	};
	Register<bit<32>, bit<16>  >(16384) elastic_stage3_2;
	RegisterAction<bit<32>, bit<16> , bit<32> >(elastic_stage3_2) elastic_stage3_2_add=
	{
		void apply(inout bit<32> register_data,out bit<32> result)
		{
		    result = 0;
			register_data = register_data + meta.output2_2;
		
		}
	};
    RegisterAction<bit<32>, bit<16> , bit<32> >(elastic_stage3_2) elastic_stage3_2_chan=
	{
		void apply(inout bit<32> register_data,out bit<32> result)
		{
		    result = register_data;
			register_data = meta.output2_2;
		}
	};

	Register<data_lrfu, bit<16> >(8192) lrfu_stage1_1;
	RegisterAction<data_lrfu, bit<16> , bit<32> >(lrfu_stage1_1) lrfu_stage1_1_insert=
	{
		void apply(inout data_lrfu register_data,out bit<32> result)
		{
		
		if(register_data.key==meta.output3_1||meta.output3_2>=register_data.pure){
			result = register_data.key;
		}
		else 
		{
			result = 0;
		}

		if(register_data.key==meta.output3_1)
		{
			register_data.pure = register_data.pure + meta.output3_2;
		}
		else
		{
			register_data.pure = max(register_data.pure, meta.output3_2);
		}

        if(meta.output3_2>=register_data.pure)
		{
			register_data.key = meta.output3_1;
		}
		
		}
	};
	Register<data_lrfu, bit<16> >(4096) lrfu_stage2_1;
	RegisterAction<data_lrfu, bit<16> , bit<32> >(lrfu_stage2_1) lrfu_stage2_1_insert=
	{
		void apply(inout data_lrfu register_data,out bit<32> result)
		{
		
		if(register_data.key==meta.output4_1||meta.output4_2>=register_data.pure){
			result = register_data.key;
		}
		else 
		{
			result = 0;
		}

		if(register_data.key==meta.output4_1)
		{
			register_data.pure = register_data.pure + meta.output4_2;
		}
		else
		{
			register_data.pure = max(register_data.pure, meta.output4_2);
		}

        if(meta.output4_2>=register_data.pure)
		{
			register_data.key = meta.output4_1;
		}
		
		}
	};
	Register<bit<32>, bit<16>  >(8192) lrfu_stage1_2;
	RegisterAction<bit<32>, bit<16> , bit<32> >(lrfu_stage1_2) lrfu_stage1_2_add=
	{
		void apply(inout bit<32> register_data,out bit<32> result)
		{
		    result = 0;
			register_data = register_data + meta.output3_2;
		
		}
	};
    RegisterAction<bit<32>, bit<16> , bit<32> >(lrfu_stage1_2) lrfu_stage1_2_chan=
	{
		void apply(inout bit<32> register_data,out bit<32> result)
		{
		    result = register_data;
			register_data = meta.output3_2;
		}
	};
	Register<bit<32>, bit<16>  >(4096) lrfu_stage2_2;
	RegisterAction<bit<32>, bit<16> , bit<32> >(lrfu_stage2_2) lrfu_stage2_2_add=
	{
		void apply(inout bit<32> register_data,out bit<32> result)
		{
		    result = 0;
			register_data = register_data + meta.output4_2;
		
		}
	};
    RegisterAction<bit<32>, bit<16> , bit<32> >(lrfu_stage2_2) lrfu_stage2_2_chan=
	{
		void apply(inout bit<32> register_data,out bit<32> result)
		{
		    result = register_data;
			register_data = meta.output4_2;
		}
	};

    action cal_index()
    {
        meta.index2=(bit<16>) hash_0.get({hdr.ipv4.src_addr,hdr.ipv4.dst_addr,hdr.ipv4.protocol,hdr.myflow.id});
    }
	@stage(0) table cal_index_t {
        actions = { cal_index; }
        default_action = cal_index;
    }
	action elastic_stage1_1_a()
    {
        meta.output1_1=elastic_stage1_1_insert.execute(hash_1.get({hdr.ipv4.src_addr,hdr.ipv4.dst_addr,hdr.ipv4.protocol,hdr.myflow.id}));
    }
    @stage(0) table elastic_stage1_1_t {
        actions = { elastic_stage1_1_a; }
        default_action = elastic_stage1_1_a;
    }
    action elastic_stage1_2_add_a()
    {
        meta.output1_2=elastic_stage1_2_add.execute(hash_1.get({hdr.ipv4.src_addr,hdr.ipv4.dst_addr,hdr.ipv4.protocol,hdr.myflow.id}));
    }
    @stage(1) table elastic_stage1_2_add_t {
        actions = { elastic_stage1_2_add_a; }
        default_action = elastic_stage1_2_add_a;
    }
    action elastic_stage1_2_chan_a()
    {
        meta.output1_2=elastic_stage1_2_chan.execute(hash_1.get({hdr.ipv4.src_addr,hdr.ipv4.dst_addr,hdr.ipv4.protocol,hdr.myflow.id}));
    }
    @stage(1) table elastic_stage1_2_chan_t {
        actions = { elastic_stage1_2_chan_a; }
        default_action = elastic_stage1_2_chan_a;
    }
	action elastic_stage2_1_a()
    {
        meta.output2_1=elastic_stage2_1_insert.execute(meta.index2);
    }
    @stage(1) table elastic_stage2_1_t {
        actions = { elastic_stage2_1_a; }
        default_action = elastic_stage2_1_a;
    }
    action elastic_stage2_2_add_a()
    {
        meta.output2_2=elastic_stage2_2_add.execute(meta.index2);
    }
    @stage(2) table elastic_stage2_2_add_t {
        actions = { elastic_stage2_2_add_a; }
        default_action = elastic_stage2_2_add_a;
    }
    action elastic_stage2_2_chan_a()
    {
        meta.output2_2=elastic_stage2_2_chan.execute(meta.index2);
    }
    @stage(2) table elastic_stage2_2_chan_t {
        actions = { elastic_stage2_2_chan_a; }
        default_action = elastic_stage2_2_chan_a;
    }
	action elastic_stage3_1_a()
    {
        meta.output3_1=elastic_stage3_1_insert.execute(meta.index3);
    }
    @stage(2) table elastic_stage3_1_t {
        actions = { elastic_stage3_1_a; }
        default_action = elastic_stage3_1_a;
    }
    action elastic_stage3_2_add_a()
    {
        meta.output3_2=elastic_stage3_2_add.execute(meta.index3);
    }
    @stage(3) table elastic_stage3_2_add_t {
        actions = { elastic_stage3_2_add_a; }
        default_action = elastic_stage3_2_add_a;
    }
    action elastic_stage3_2_chan_a()
    {
        meta.output3_2=elastic_stage3_2_chan.execute(meta.index3);
    }
    @stage(3) table elastic_stage3_2_chan_t {
        actions = { elastic_stage3_2_chan_a; }
        default_action = elastic_stage3_2_chan_a;
    }
	
	action lrfu_stage1_1_a()
    {
        meta.output4_1=lrfu_stage1_1_insert.execute(meta.index4);
    }
    @stage(4) table lrfu_stage1_1_t {
        actions = { lrfu_stage1_1_a; }
        default_action = lrfu_stage1_1_a;
    }
    action lrfu_stage1_2_add_a()
    {
        meta.output4_2=lrfu_stage1_2_add.execute(meta.index4);
    }
    @stage(5) table lrfu_stage1_2_add_t {
        actions = { lrfu_stage1_2_add_a; }
        default_action = lrfu_stage1_2_add_a;
    }
    action lrfu_stage1_2_chan_a()
    {
        meta.output4_2=lrfu_stage1_2_chan.execute(meta.index4);
    }
    @stage(5) table lrfu_stage1_2_chan_t {
        actions = { lrfu_stage1_2_chan_a; }
        default_action = lrfu_stage1_2_chan_a;
    }
	action lrfu_stage2_1_a()
    {
        meta.output5_1=lrfu_stage2_1_insert.execute(meta.index5);
    }
    @stage(6) table lrfu_stage2_1_t {
        actions = { lrfu_stage2_1_a; }
        default_action = lrfu_stage2_1_a;
    }
    action lrfu_stage2_2_add_a()
    {
        meta.output5_2=lrfu_stage2_2_add.execute(meta.index5);
    }
    @stage(7) table lrfu_stage2_2_add_t {
        actions = { lrfu_stage2_2_add_a; }
        default_action = lrfu_stage2_2_add_a;
    }
    action lrfu_stage2_2_chan_a()
    {
        meta.output5_2=lrfu_stage2_2_chan.execute(meta.index5);
    }
    @stage(7) table lrfu_stage2_2_chan_t {
        actions = { lrfu_stage2_2_chan_a; }
        default_action = lrfu_stage2_2_chan_a;
    }
    Random<bit<2>>() random_generator;

	action generate_random_number(){
		meta.rng = random_generator.get();
	}

	table random_number_table{
		actions = {
			generate_random_number;
		}
		size = 1;
		const default_action = generate_random_number();
	}
	/* ingress processing*/
	apply{
        random_number_table.apply();
        if(meta.rng==1){
            meta.id = meta.id >> 8;
        }else if(meta.rng==2){
            meta.id = meta.id >> 16;
        }else{
            meta.id = meta.id >> 24;
        }
        if (hdr.myflow.isValid()) {
        	cal_index_t.apply();
			elastic_stage1_1_t.apply();
			if(meta.output1_1==meta.id)
			elastic_stage1_2_add_t.apply();
			else {
				elastic_stage1_2_chan_t.apply();
				elastic_stage2_1_t.apply();
				meta.index3 = meta.index2 >> 1;
				if(meta.output2_1==meta.output1_1)
				elastic_stage2_2_add_t.apply();
				else {
					elastic_stage2_2_chan_t.apply();
					elastic_stage3_1_t.apply();
					meta.index4 = meta.index2 >> 2;
					if(meta.output3_1==meta.output2_1)
					elastic_stage3_2_add_t.apply();
					else {
						elastic_stage3_2_chan_t.apply();
						lrfu_stage1_1_t.apply();
						meta.index5 = meta.index2 >> 3;
						if(meta.output4_1==meta.output3_1)
						lrfu_stage1_2_add_t.apply();
						else {
							lrfu_stage1_2_chan_t.apply();
							lrfu_stage2_1_t.apply();
							if(meta.output5_1==meta.output4_1)
							lrfu_stage2_2_add_t.apply();
							else {
								lrfu_stage2_2_chan_t.apply();
							}
						}
					}
				}
			}
		}
	}
}

control IngressDeparser(packet_out pkt,
    /* User */
    inout my_ingress_headers_t                       hdr,
    in    my_ingress_metadata_t                      meta,
    /* Intrinsic */
    in    ingress_intrinsic_metadata_for_deparser_t  ig_dprsr_md)
{
    apply {
        pkt.emit(hdr);
    }
}

/*************************************************************************
 ****************  E G R E S S   P R O C E S S I N G   *******************
 *************************************************************************/

    /***********************  H E A D E R S  ************************/

struct my_egress_headers_t {
}

    /********  G L O B A L   E G R E S S   M E T A D A T A  *********/

struct my_egress_metadata_t {
}

    /***********************  P A R S E R  **************************/

parser EgressParser(packet_in        pkt,
    /* User */
    out my_egress_headers_t          hdr,
    out my_egress_metadata_t         meta,
    /* Intrinsic */
    out egress_intrinsic_metadata_t  eg_intr_md)
{
    /* This is a mandatory state, required by Tofino Architecture */
    state start {
        pkt.extract(eg_intr_md);
        transition accept;
    }
}

    /***************** M A T C H - A C T I O N  *********************/

control Egress(
    /* User */
    inout my_egress_headers_t                          hdr,
    inout my_egress_metadata_t                         meta,
    /* Intrinsic */    
    in    egress_intrinsic_metadata_t                  eg_intr_md,
    in    egress_intrinsic_metadata_from_parser_t      eg_prsr_md,
    inout egress_intrinsic_metadata_for_deparser_t     eg_dprsr_md,
    inout egress_intrinsic_metadata_for_output_port_t  eg_oport_md)
{
    apply {
    }
}

    /*********************  D E P A R S E R  ************************/

control EgressDeparser(packet_out pkt,
    /* User */
    inout my_egress_headers_t                       hdr,
    in    my_egress_metadata_t                      meta,
    /* Intrinsic */
    in    egress_intrinsic_metadata_for_deparser_t  eg_dprsr_md)
{
    apply {
        pkt.emit(hdr);
    }
}



/************ F I N A L   P A C K A G E ******************************/
Pipeline(
    IngressParser(),
    Ingress(),
    IngressDeparser(),
    EgressParser(),
    Egress(),
    EgressDeparser()
) pipe;

Switch(pipe) main;
