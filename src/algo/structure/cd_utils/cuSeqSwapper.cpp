/* $Id$
 * ===========================================================================
 *
 *                            PUBLIC DOMAIN NOTICE
 *               National Center for Biotechnology Information
 *
 *  This software/database is a "United States Government Work" under the
 *  terms of the United States Copyright Act.  It was written as part of
 *  the author's official duties as a United States Government employee and
 *  thus cannot be copyrighted.  This software/database is freely available
 *  to the public for use. The National Library of Medicine and the U.S.
 *  Government have not placed any restriction on its use or reproduction.
 *
 *  Although all reasonable efforts have been taken to ensure the accuracy
 *  and reliability of the software and data, the NLM and the U.S.
 *  Government do not and cannot warrant the performance or results that
 *  may be obtained by using this software or data. The NLM and the U.S.
 *  Government disclaim all warranties, express or implied, including
 *  warranties of performance, merchantability or fitness for any particular
 *  purpose.
 *
 *  Please cite the author in any work or product based on this material.
 *
 * ===========================================================================
 *
 * Author:  Charlie Liu
 *
 * File Description:
 *
 *       Replace local sequences in a CD
 *
 * ===========================================================================
 */

#include <ncbi_pch.hpp>

#include <algo/structure/cd_utils/cuSeqSwapper.hpp>
#include <algo/structure/cd_utils/cuSeqTreeFactory.hpp>
#include <algo/structure/cd_utils/cuBlast2Seq.hpp>
#include <algo/structure/cd_utils/cuCD.hpp>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(cd_utils)
SeqSwapper::SeqSwapper(CCdCore* cd, int identityThreshold):
	m_cd(cd), m_clusteringThreshold(identityThreshold), m_replacingThreshold(identityThreshold), m_ac(cd, CCdCore::USE_ALL_ALIGNMENT)
{
}
	
void SeqSwapper::swapSequences()
{
	int numNormal = m_cd->GetNumRows();
	
	//debug
	/*
	int numLocal = 0;
	for (int i = 0; i < numNormal; i++)
	{
		CRef< CSeq_id > seqId;
		m_ac.GetSeqIDForRow(i, seqId);
		if (seqId->IsLocal())
		{
			LOG_POST("found local seqId at row "<<i);
			numLocal++;
		}
	}
	if (numLocal == 0)LOG_POST("Could not find any local seq-id");
	int num = m_ac.GetNumRows();
	*/

	vector< vector<int> * > clusters;
	makeClusters(m_clusteringThreshold, clusters);
	vector< pair<int, int> > replacementPairs;
	set<int> structures;

	for (int i = 0; i < clusters.size(); i++)
	{
		vector<int>* cluster = clusters[i];
		findReplacements(*cluster, replacementPairs, structures);
		delete cluster;
	}
	set<int> usedPendings;
	
	vector<int> selectedNormalRows;
	int newMaster = -1;
	for (int p = 0; p < replacementPairs.size(); p++)
	{
		//debug
		CRef< CSeq_id > seqId;
		m_ac.GetSeqIDForRow(replacementPairs[p].first, seqId);
		string nid = seqId->AsFastaString();
		m_ac.GetSeqIDForRow(replacementPairs[p].second, seqId);
		string pid = seqId->AsFastaString();
		LOG_POST("replacing "<<nid<<" with "<<pid);
		//take care of master replacement
		if (replacementPairs[p].first == 0)
			newMaster = replacementPairs[p].second - numNormal;
		else
		{
			selectedNormalRows.push_back(replacementPairs[p].first);
			usedPendings.insert(replacementPairs[p].second - numNormal);
		}
	}
	m_cd->EraseTheseRows(selectedNormalRows);
	if (newMaster >= 0)
		promotePendingRows(usedPendings, &newMaster);
	else
		promotePendingRows(usedPendings);
	//findStructuralPendings(structures);
	if (structures.size() > 0)
		promotePendingRows(structures);
	if (newMaster > 0)
	{
		ReMasterCdWithoutUnifiedBlocks(m_cd, newMaster, true);
		vector<int> rows;
		rows.push_back(newMaster);
		m_cd->EraseTheseRows(rows);
	}
	m_cd->ResetPending();
	m_cd->EraseSequences();
}

void SeqSwapper::makeClusters(int identityThreshold, vector< vector<int> * >& clusters)
{
	TreeOptions option;
	option.clusteringMethod = eSLC;
	option.distMethod = ePercentIdentityRelaxed;
	SeqTree* seqtree = TreeFactory::makeTree(&m_ac, option);
	if (seqtree)
	{
		seqtree->prepare();
		//seqtree->fixRowNumber(m_ac);
		double pid = ((double)identityThreshold)/100.0;
		double distTh = 1.0 -  pid;
		double distToRoot = seqtree->getMaxDistanceToRoot() - distTh;
		vector<SeqTreeIterator> nodes;
		seqtree->getDistantNodes(distToRoot, nodes);
		for (int i = 0; i < nodes.size(); i++)
		{
			vector<int>* rowids = new vector<int>;
			seqtree->getSequenceRowid(nodes[i], *rowids);
			clusters.push_back(rowids);
		}
	}
}

void SeqSwapper::findReplacements(vector<int>& cluster, vector< pair<int,int> >& replacementPairs, set<int>& structs)
{
	if (cluster.size() == 0)
		return;
	// seperate normal from pending
	vector<int> normal, pending;
	set<int> pending3D;
	for (int i = 0; i < cluster.size(); i++)
	{
		if (m_ac.IsPending(cluster[i]))
		{
			pending.push_back(cluster[i]);
			if (m_ac.IsPdb(cluster[i]))
			{
				pending3D.insert(cluster[i]);
			}
		}
		else
		{
			//only care about local seq_id for now
			CRef< CSeq_id > seqId;
			m_ac.GetSeqIDForRow(cluster[i], seqId);
			if (seqId->IsLocal())
				normal.push_back(cluster[i]);
		}
	}
	if ((normal.size() == 0) || (pending.size() == 0))
		return;
	//blast normal against pending
	CdBlaster blaster(m_ac);
	blaster.setQueryRows(&normal);
	blaster.setSubjectRows(&pending);
	blaster.setScoreType(CSeq_align::eScore_IdentityCount);
	blaster.blast();
	//for each normal, find the hightest-scoring (in identity), unused and above the threshold pending
	set<int> usedPendingIndice;
	for (int n = 0; n < normal.size(); n++)
	{
		int maxId = 0;
		int maxIdIndex = -1;
		for (int p = 0; (p < pending.size()) && (usedPendingIndice.find(p) == usedPendingIndice.end()); p++)
		{
			int pid = (int)blaster.getPairwiseScore(n, p);
			if ((pid > maxId) && (pid >= m_replacingThreshold))
			{
				maxId = pid;
				maxIdIndex = p;
			}
		}
		if ( maxIdIndex >= 0)
		{
			usedPendingIndice.insert(maxIdIndex);
			replacementPairs.push_back(pair<int, int>(normal[n], pending[maxIdIndex]));
			set<int>::iterator sit = pending3D.find(pending[maxIdIndex]);
			if (sit != pending3D.end())
			{
				pending3D.erase(sit); //this 3D has been used.
			}
		}
	}
	if (pending3D.size() > 0)
	{
		structs.insert(pending3D.begin(), pending3D.end());
	}
}


void SeqSwapper::findStructuralPendings(set<int>& rows)
{
	AlignmentCollection ac(m_cd);// default, pending only
	int num = ac.GetNumRows();
	for (int i = 0; i < num; i++)
		if (ac.IsPdb(i))
			rows.insert(i);
}

void SeqSwapper::promotePendingRows(set<int>& rows, int* newMaster)
{
	AlignmentCollection ac(m_cd);// default, pending only
	int masterInPending = -1;
	if (newMaster)
	{
		masterInPending = *newMaster;
		m_cd->AddSeqAlign(ac.getSeqAlign(*newMaster));
		*newMaster = m_cd->GetNumRows() - 1;
	}
	for (set<int>::iterator sit = rows.begin(); sit != rows.end(); sit++)
	{
		m_cd->AddSeqAlign(ac.getSeqAlign(*sit));
	}
	if (masterInPending >= 0)
	rows.insert(masterInPending);
	m_cd->ErasePendingRows(rows);
}

END_SCOPE(cd_utils)
END_NCBI_SCOPE

