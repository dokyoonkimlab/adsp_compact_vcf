#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <iterator>
#include <cmath>

const int rm_sample {5202}; // remove sample 5202 (A-ADC-AD005043-BR-NCR-09AD15795) as it is removed from preview to QC version of vcf.

struct Variant {
	// the CHROM, ID, REF, ALT, QUAL, FILTER, INFO, and FORMAT columns in VCF
	std::string chr, id, ref, alt, qual, filter, info, format;
	int pos, info_ac, info_an, info_dp; // position, AC, AN, and DP in INFO.
	int qc_fail_dp, qc_fail_gq, qc_fail_both; // DP < 10 and/or GQ <20.
	int ac_qc, an_qc, dp_qc; // AC and AN after QC.
	int ad_het_0, ad_het_dp; // Store AD counts for heterogeneous SNPs
	int gt_total, gt_not_miss; // To calculate call rate after filter DP < 10 and GQ < 20
	std::vector<std::string> gt;
	std::vector<std::string> dp;
	std::vector<std::string> gq;
	bool pass;
	bool all_fail_dp_gq;

	bool clean() {
		chr.clear();
		id.clear();
		ref.clear();
		alt.clear();
		qual.clear();
		filter.clear();
		info.clear();
		format.clear();
		pos=0, info_ac=0, info_an=0, info_dp=0;
		qc_fail_dp=0, qc_fail_gq=0, qc_fail_both=0;
		ac_qc=0, an_qc=0, dp_qc=0;
		ad_het_0=0, ad_het_dp=0;
		gt_total=0, gt_not_miss=0;
		gt.clear();
		dp.clear();
		gq.clear();
		pass = false;
		all_fail_dp_gq = false;

		return true;
	}
};

//Get the basic info the first to nineth columns in VCF
inline void GetBasicInfo (Variant & var, std::stringstream & ss) {
	ss >> var.chr;
	ss >> var.pos;
	ss >> var.id;
	ss >> var.ref;
	ss >> var.alt;
	ss >> var.qual;
	ss >> var.filter;
	ss >> var.info;
	ss >> var.format;

	std::size_t found1 = var.info.find("AC=");
	std::size_t found2 = var.info.find(";", found1 + 1);
	var.info_ac = std::atoi(var.info.substr(found1 + 3, found2 - found1 - 3).c_str());

	found1 = var.info.find("AN=");
	found2 = var.info.find(";", found1 + 1);
	var.info_an = std::atoi(var.info.substr(found1 + 3, found2 - found1 - 3).c_str());

	found1 = var.info.find("DP=");
	found2 = var.info.find(";", found1 + 1);
	var.info_dp = std::atoi(var.info.substr(found1 + 3, found2 - found1 - 3).c_str());
}

//Add "ori" as a prefix to AN, AC, AF, and DP in INFO field.
inline void AddPrefix (const std::string & prefix, const std::string & target, std::string & info) {
	const std::size_t found = info.find(target);
	if (found != std::string::npos)
		info.insert(found, prefix);
	else
		std::cerr << "Warning: Cannot find " << target << " in " << info << std::endl;
		
}

// Keep falgs and ABHet in INFO and discard others
// Return True if the variant is good.
bool KeepFlag (std::string & info) {
	std::stringstream ss(info);
	std::string token, tmp;
	bool all_flagged = true, all_het_fail = true;
	while (std::getline(ss, token, ';')) { // Divid by ';'
		if (token.find ("VFLAGS_") != std::string::npos ||
			token.find ("ABHet_") != std::string::npos) { // Only keep element with VFLAGS_ and ABHet_
			if (!tmp.empty()) tmp += (';' + token); // Not the first element, needs ';'
			else tmp = token; // the first element in INFO

			if (token.find ("VFLAGS_") != std::string::npos) {
				std::size_t pos = token.find('=');
				if (token.substr(pos + 1) == "0")
					all_flagged = false;
			}

			if (token.find ("ABHet_") != std::string::npos) {
				std::size_t pos = token.find('=');
				float het = std::stod(token.substr(pos + 1));
				if (het > 0.25 && het < 0.75)
					all_het_fail = false;
			}
		}
	}

	info = tmp; // Replace the old info
	//return !(all_flagged & all_het_fail);
	return !all_flagged;
}

//Extract GT,DP and GQ values of each sample and push them to var.gt, var.dp, and var.gq.
bool ExtractInfo (Variant & var, std::stringstream & ss) {
	//var.ac = 0;
	//var.an = 0;
	std::string token;
	//bool all_missing = true;
	bool first = true;
	int sample_index = 0;
	int all_fail_dp_gq = 0;
	const std::string missingGt = "./.";
	while (std::getline(ss, token, '\t')) { // Divid by tab
		if (first) { // The first one is the empty one.
			first = false;
			continue;
		}
		sample_index++;
		if (sample_index == rm_sample) continue; // remove sample
		//std::cerr << token << std::endl;
		std::size_t pos = token.find(':'); // GT is the first info in ADSP VCF
		const std::string gt = token.substr(0, pos); // Extract GT
		var.gt.push_back(gt); // Store GT

		/*
		all_missing &= (gt == "./.");
		*/

		int an = 0, ac = 0;

		if (gt[0] != '.') an++;
		if (gt[2] != '.') an++;
		if (gt[0] != '.' && gt[0] != '0') ac++;
		if (gt[2] != '.' && gt[2] != '0') ac++;

		var.an_qc += an;
		var.ac_qc += ac;
		
		// For AD
		std::size_t pos_ad = token.find(':', pos + 1); // AD
		const std::string ad = token.substr(pos + 1, pos_ad - pos - 1).c_str();

		// For DP
		std::size_t pos1 = token.find((':'), pos_ad + 1); // DP
		//var.dp.push_back(token.substr(pos + 1, pos1 - pos - 1));
		const int dp = std::atoi(token.substr(pos_ad + 1, pos1 - pos_ad - 1).c_str());
		//std::cerr << std::atoi(token.substr(pos + 1, pos1 - pos - 1).c_str()) << std::endl;
		const bool qc_dp_fail = (dp < 10) ? true : false;
		if (qc_dp_fail) {
			var.qc_fail_dp++;
			var.an_qc -= an;
			var.ac_qc -= ac;
			*(var.gt.rbegin()) = missingGt;
			all_fail_dp_gq++;
		}

		// For GQ
		pos = pos1;
		pos1 = token.find((':'), pos + 1); // GQ
		//var.gq.push_back(token.substr(pos + 1, pos1 - pos - 1));
		const bool qc_gq_fail = (std::atoi(token.substr(pos + 1, pos1 - pos - 1).c_str()) < 20) ? true : false;
		//std::cerr << std::atoi(token.substr(pos + 1, pos1 - pos - 1).c_str()) << std::endl;
		//const bool qc_gq_fail = false;
		if (qc_gq_fail) var.qc_fail_gq++;
		if (qc_gq_fail && !qc_dp_fail) {
			var.an_qc -= an;
			var.ac_qc -= ac;
			*(var.gt.rbegin()) = missingGt;
			all_fail_dp_gq ++;
		}

		if (!qc_dp_fail && !qc_gq_fail) {
			var.dp_qc += dp;
			if (gt[0] != gt[2]) {
				std::size_t pos = ad.find(',');
				var.ad_het_0 += std::atoi(ad.substr(0, pos).c_str());
				var.ad_het_dp += dp;
			}
			if (gt[0] != '.' && gt[2] != '.') var.gt_not_miss++;
		} 
		var.gt_total++;
		
		if (qc_dp_fail && qc_gq_fail) var.qc_fail_both++;
	}
	if (all_fail_dp_gq == sample_index - 1) var.all_fail_dp_gq = 1;
	//return all_missing;
	return false;
}

void PrintVariant(const Variant & var, const bool filter_variants) {
	//if (!var.pass) return;
	const float af = (var.an_qc == 0) ? 0 : var.ac_qc/var.an_qc;
	const float abhet = (var.ad_het_0 == 0) ? -1 : round((float(var.ad_het_0) / var.ad_het_dp * 10000)) / 10000;

	std::ostringstream abhet_ss;
	if (abhet == -1) abhet_ss << ".";
	else abhet_ss << abhet;

	std::string vflags = "";
	bool flag_added = 0;
	if (var.filter != "PASS"){
		vflags.append("1");
		flag_added = 1;
	}
	if (var.all_fail_dp_gq) {
		flag_added ? vflags.append(",2") : vflags.append("2");
		flag_added = 1;		
	}
	if (var.ac_qc == 0 || var.ac_qc == var.an_qc) {   // account for monomorphic 0/0 or monomorphic 1/1
		flag_added ? vflags.append(",3") : vflags.append("3");
		flag_added = 1;
	}
	const float missingness = (float(var.gt_not_miss) / var.gt_total);
	if (var.gt_total > 0 && missingness - 0.8f < 1e-8) {
		flag_added ? vflags.append(",4") : vflags.append("4");
		flag_added = 1;
	} 
	if (!flag_added) vflags.append("0");

	if (!filter_variants || (filter_variants && !flag_added && abhet > 0.25f && abhet < 0.75f)) {
		std::cout << var.chr << "\t"
			<< var.pos << "\t"
			<< var.id << "\t"
			<< var.ref << "\t"
			<< var.alt << "\t"
			<< var.qual << "\t"
			<< var.filter << "\t"
			//<< var.info << "\t"
			<< "AC=" << var.ac_qc << ";AF=" << af << ";AN=" << var.an_qc << ";DP=" << var.dp_qc << ";" << var.info << ";VFLAGS_one_subgroup=" << vflags << ";ABHet_one_subgroup=" << abhet_ss.str() << "\t"
			<< var.format;
		// Print GT of each sample
		for (unsigned int i = 0; i < var.gt.size(); ++i) // The first one GT is empty, so we skip it.
			std::cout << "\t" << var.gt[i];
		std::cout << std::endl;
	}
}

int main (int argc, char** argv) {

	// USAGE: zcat vcf.gz | <PROGRAM> 2> info.txt | gzip > output.vcf.gz

	std::ios_base::sync_with_stdio(false); // Fastern IO
	std::cin.tie(NULL); // Fastern IO

	std::string line; // buffer
	//std::cerr << "ORI_AC\tORI_AN\tORI_DP\tQC_AC\tQC_AN\tQC_DP\tDP<10\tGQ<20\tDP<10_AND_GQ<20" << std::endl;
	while (std::getline(std::cin, line)) { // Catch a line from stdin
		if (line[0] == '#') { // for header section, write out directly
			if (line.rfind("#CHROM", 0) == 0) {

				//Add header lines for VFLAGS_one_subgroup and ABHet_one_subgroup.
				std::cout << "##INFO=<ID=VFLAGS_one_subgroup,Number=.,Type=Integer,Description=\"Pipeline-specific QC variant flags\">" << std::endl;
				std::cout << "##INFO=<ID=ABHet_one_subgroup,Number=1,Type=Float,Description=\"Allelic Read Ratio\">" << std::endl;

				std::istringstream iss(line);
				std::string token;
				int sample_index = -9;
				bool first = 1;
				while (std::getline(iss, token, '\t')) {
					sample_index++;
					if (sample_index != rm_sample) {
						if (first) {
							std::cout << token;
							first = 0;
						} else std::cout << '\t' << token;
					} else std::cerr << "removing sample " << token << std::endl;
				}
				std::cout << std::endl;
			} else {
				std::cout << line << std::endl;
			}
			// Insert new tags in INFO field
			if (line.find("ID=AC") != std::string::npos || line.find("ID=AN") != std::string::npos
				|| line.find("ID=AF") != std::string::npos || line.find("ID=DP") != std::string::npos) {
				const std::size_t found = line.find("ID=");
				line.insert(found + 3, "ori_");
				std::cout << line << std::endl;
			}
		} else {
			std::stringstream ss(line); // Convert line to stringstream
			Variant var;
			var.clean();
			GetBasicInfo(var, ss);
			AddPrefix("ori_", "AC=", var.info);
			AddPrefix("ori_", "AN=", var.info);
			AddPrefix("ori_", "AF=", var.info);
			AddPrefix("ori_", "DP=", var.info);
			var.format = "GT"; // Only keep GT
			//var.pass = KeepFlag(var.info); // rephase info; keep only VFLAGS and ABHet
			ExtractInfo(var, ss); // Change low-qual genotypes to missing
			PrintVariant(var, 1); // Output vcf to stdout
			//std::cerr << var.info_ac << "\t" << var.info_an << "\t" << var.info_dp << "\t" << var.ac_qc << "\t" 
			//<< var.an_qc << "\t" << var.dp_qc << "\t" << var.qc_fail_dp << "\t" << var.qc_fail_gq << "\t" << var.qc_fail_both << std::endl;
		}
	}

	std::cerr << "Program is done normally." << std::endl;
	return 0;
}
