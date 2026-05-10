# Plagrism-Detection

A Complete Plagiarism Detection Built in CPP. The system compares documents using a multi-algorithmic approach ranging from exact string matching (KMP, Rabin-Karp, Naive) to structural similarity and sequence alignment (LCS, W-Shingling).

Each algorithm in this project serves a specific purpose in the detection pipeline:

      1. KMP (Knuth-Morris-Pratt): 
              Optimized exact pattern matching that uses a "Longest Prefix Suffix" (LPS) table to avoid redundant comparisons.
              Complexity: $O(n + m)$.
              
      2. Rabin-Karp: 
              Employs Rolling Hash logic to find patterns. Excellent for detecting multiple plagiarism signatures simultaneously.
              Complexity: Average $O(n + m)$, Worst $O(n \times m)$.

      3. LCS (Longest Common Subsequence): 
              Used to identify the longest string of characters that appear in the same relative order in both documents. 
              This is highly effective for detecting "shuffled" plagiarism.
              Complexity: $O(n \times m)$ using Dynamic Programming.

      4. W-Shingling: 
              Breaks documents into contiguous sets of $n$-tokens (shingles) to calculate the Jaccard Similarity Coefficient. 
              This detects structural similarity even if words are slightly altered.

      5. Naive String Matching: 
              Used as a baseline to compare the performance and optimization gains of the more advanced algorithms.
