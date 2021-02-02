**Note:** The program is specifically designed for R3 release of [The Alzheimer's Disease Sequencing Project (ADSP)](https://www.niagads.org/adsp/).
The purpose is to reverse GT (genotype) of each sample for each variant to reduce the file size of VCFs. compact_filter is a program to further replace low-quality GT (DP<10 or GQ<20) by missing (./.) that is suggested by the GCAD/ADSP QC working group.  The input VCF is expected generated  by GATK v4.

## Getting started

	git clone https://bitbucket.org/NIAGADS/compact_vcf.git
	cd compact_vcf
	g++ -O3 -std=c++11 -o compact_vcf compact_vcf.cpp
	g++ -O3 -std=c++11 -o compact_filter_vcf compact_filter_vcf.cpp

## Usage
Compact VCF

	zcat <VCF> | compact_vcf | gzip > <VCF>.compact.vcf.gz

Compact VCF and replace low-quality GT by ./.
The log file is tab-delimited and shows the allele counts and numbers before (ORI) and after (QC) replacing GTs.

	ORI_AC	ORI_AN	ORI_DP	QC_AC	QC_AN	QC_DP	DP<10	GQ<20	DP<10_AND_GQ<20

We also updated the allele frequencies, numbers and counts (AF, AN and AC) in the INFO field of VCF while reserved original AF, AN and AC from GATK as ori_AF, ori_AN and ori_AC.

	zcat <VCF> | compact_filter_vcf 2> <VCF>.log | gzip > <VCF>.compact.vcf.gz

