// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2025
#include "ayu/epstein_mode.h"

#include <QtCore/QRegularExpression>
#include <random>
#include <vector>
#include <algorithm>
#include <set>

namespace Ayu::Epstein {

namespace {

struct WordSpan {
	int start = 0;
	int length = 0;
	QString text;
};

struct Range {
	int start = 0;
	int end = 0; // Exclusive
};

bool IsProtected(int start, int end, const std::vector<Range> &protectedRanges) {
	for (const auto &range : protectedRanges) {
		// Check for intersection
		if (start < range.end && range.start < end) {
			return true;
		}
	}
	return false;
}

} // namespace

QString Obfuscate(const QString &text) {
	if (text.isEmpty()) {
		return text;
	}

	// 1. Find Protected Ranges
	std::vector<Range> protectedRanges;

	// URL: (?i)\b(?:https?://|tg://|ftp://|www\.)\S+
	// QRegularExpression handles (?i) via CaseInsensitiveOption if needed, but we can put it in pattern too.
	// Note: \b in regex means word boundary.
	static const QRegularExpression urlRegex(
		R"((?i)\b(?:https?://|tg://|ftp://|www\.)\S+)",
		QRegularExpression::CaseInsensitiveOption);
	
	// Email: (?i)\b\S+ @\S+\b
	static const QRegularExpression emailRegex(
		R"((?i)\b\S+@\S+\b)",
		QRegularExpression::CaseInsensitiveOption);

	// Mentions/Hashtags: (?<!\S)[ @#][\p{L}\p{Nd}_]{2,}
	static const QRegularExpression mentionRegex(
		R"((?<!\S)[@#][\p{L}\p{Nd}_]{2,})");

	auto addMatches = [&](const QRegularExpression &re) {
		auto i = re.globalMatch(text);
		while (i.hasNext()) {
			auto match = i.next();
			protectedRanges.push_back({ (int)match.capturedStart(), (int)match.capturedEnd() });
		}
	};

	addMatches(urlRegex);
	addMatches(emailRegex);
	addMatches(mentionRegex);

	// 2. Identify Candidate Words
	// Regex: [\p{L}\p{Mn}\p{Nd}_']+
	static const QRegularExpression wordRegex(R"([\p{L}\p{Mn}\p{Nd}_']+)");

	std::vector<WordSpan> allWords;
	auto i = wordRegex.globalMatch(text);
	while (i.hasNext()) {
		auto match = i.next();
		WordSpan span;
		span.start = (int)match.capturedStart();
		span.length = (int)match.capturedLength();
		span.text = match.captured();
		allWords.push_back(span);
	}

	std::vector<int> candidateIndices; // Indices into allWords
	static const std::set<QString> whitelist = {
		"epstein", "uzbek", "uzbekgram", "узбекграм"
	};

	for (size_t k = 0; k < allWords.size(); ++k) {
		const auto &w = allWords[k];
		
		// 1. Not Protected
		if (IsProtected(w.start, w.start + w.length, protectedRanges)) {
			continue;
		}

		// 2. Contains at least one letter
		bool hasLetter = false;
		for (const auto &ch : w.text) {
			if (ch.isLetter()) {
				hasLetter = true;
				break;
			}
		}
		if (!hasLetter) continue;

		// 3. Length >= 3
		if (w.length < 3) continue;

		// 4. Not in Whitelist
		if (whitelist.count(w.text.toLower())) continue;

		candidateIndices.push_back(k);
	}

	if (candidateIndices.empty()) {
		return text;
	}

	// 3. Selection Logic
	// Target: 45%
	// Min Visible: 1 if total < 4, else 2.
	
	int N = (int)candidateIndices.size();
	int minVisible = (N < 4) ? 1 : 2;
	int target = (int)(N * 0.45);
	
	// Formula: Target = min(Target, N - MinVisible)
	// Actually, we want to MASK Target words. So we leave N - Target visible.
	// Wait, spec says: "Always leaving visible minimum 1 word... or 2 words..."
	// So MaxMasked = N - MinVisible.
	if (target > N - minVisible) {
		target = std::max(0, N - minVisible);
	}

	// First/Last Protection
	// Spec: "If words >= 3, first (index 0) and last (index N-1) word excluded from candidates"
	// But "candidates" here refers to "words available for masking".
	// The candidateIndices array contains indices into allWords. 
	// But wait, the spec says "first (index 0) and last (index N-1) word".
	// Does it mean index in `candidateIndices` or `allWords`?
	// "excluded from list of candidates for crossing out".
	// Assuming it refers to the list of *valid* words we just built.

	std::vector<int> availableForMasking;
	if (N >= 3) {
		// Exclude first and last of the *candidates*
		for (int k = 1; k < N - 1; ++k) {
			availableForMasking.push_back(candidateIndices[k]);
		}
	} else {
		// Use all
		availableForMasking = candidateIndices;
	}

	// Shuffle
	static std::mt19937 rng(std::random_device{}());
	std::shuffle(availableForMasking.begin(), availableForMasking.end(), rng);

	std::set<int> maskedIndices; // Indices into allWords that are masked
	
	// Anti-Cluster map to check adjacency efficiently
	// We need to know adjacency in the original text (or rather sequence of words).
	// But adjacency is defined by index in `allWords`? No, likely adjacency in the sequence of *candidates*.
	// Spec: "If Word[i-1] masked AND Word[i+1] masked - skip Word[i]".
	// This implies we look at the sequence of candidates.
	
	// Let's map candidateIndex -> masked status.
	// But since we are iterating shuffled list, we need to check neighbors.
	// Neighbors in terms of *position* in the text.
	
	// Let's use a set to track which *candidate index* (0..N-1) is masked.
	std::set<int> maskedCandidatePositions;

	int maskedCount = 0;
	for (int wordIdxInAllWords : availableForMasking) {
		if (maskedCount >= target) break;

		// Find position of this word in candidateIndices
		// (This is O(N) inside loop, making it O(N^2), but N is small (message length))
		// Optimization: Store pair<indexInAllWords, indexInCandidates> in availableForMasking?
		
		int k = -1;
		for (int i = 0; i < N; ++i) {
			if (candidateIndices[i] == wordIdxInAllWords) {
				k = i;
				break;
			}
		}
		
		if (k == -1) continue; // Should not happen

		// Anti-Cluster check
		// Check if k-1 is masked AND k+1 is masked?
		// "Masking current word should not create a chain of 3+ masked words"
		// Logic: If (k-1 masked) AND (k+1 masked), then masking k creates (k-1, k, k+1) chain.
		// Also need to check if masking k joins two existing chains?
		// Spec says: "If Word[i-1] masked AND Word[i+1] masked - skip".
		// Also check: Is k-1 and k-2 masked? Then k makes 3.
		// Is k+1 and k+2 masked? Then k makes 3.
		
		bool prevMasked = maskedCandidatePositions.count(k - 1);
		bool nextMasked = maskedCandidatePositions.count(k + 1);
		bool prevPrevMasked = maskedCandidatePositions.count(k - 2);
		bool nextNextMasked = maskedCandidatePositions.count(k + 2);

		if (prevMasked && nextMasked) continue; // ... X [X] X ...
		if (prevMasked && prevPrevMasked) continue; // X X [X] ...
		if (nextMasked && nextNextMasked) continue; // ... [X] X X

		// Pass
		maskedCandidatePositions.insert(k);
		maskedIndices.insert(wordIdxInAllWords);
		maskedCount++;
	}

	// 4. Rendering & Reconstruction
	QString result;
	int lastPos = 0;

	for (size_t k = 0; k < allWords.size(); ++k) {
		const auto &w = allWords[k];
		
		// Append text between words
		if (w.start > lastPos) {
			result.append(text.mid(lastPos, w.start - lastPos));
		}

		if (maskedIndices.count((int)k)) {
			// Mask
			int len = w.length;
			int blockCount = std::clamp(len, 3, 12);
			// Full Block █ (\u2588)
			result.append(QString(blockCount, QChar(0x2588)));
		} else {
			// Original
			result.append(w.text);
		}

		lastPos = w.start + w.length;
	}

	// Append remaining text
	if (lastPos < text.length()) {
		result.append(text.mid(lastPos));
	}

	return result;
}

} // namespace Ayu::Epstein
