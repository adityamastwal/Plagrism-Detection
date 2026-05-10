from flask import Flask, request, jsonify
from flask_cors import CORS
import os
import re
import subprocess
import requests
from bs4 import BeautifulSoup
from ddgs import DDGS
import wikipediaapi
import pdfplumber
import docx

app = Flask(__name__)
CORS(app)

UPLOAD_FOLDER = "uploads"
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

ALLOWED_EXTENSIONS = {"pdf", "doc", "docx", "txt"}

HEADERS = {
    "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36",
    "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
    "Accept-Language": "en-US,en;q=0.5",
}

wiki = wikipediaapi.Wikipedia(
    user_agent="PlagDetect/1.0 (plagiarism detection project)",
    language="en"
)


def allowed_file(filename):
    return "." in filename and filename.rsplit(".", 1)[1].lower() in ALLOWED_EXTENSIONS


def extract_text(filepath, ext):
    text = ""
    if ext == "pdf":
        with pdfplumber.open(filepath) as pdf:
            for page in pdf.pages:
                page_text = page.extract_text()
                if page_text:
                    text += page_text + "\n"
    elif ext in ("doc", "docx"):
        doc = docx.Document(filepath)
        for para in doc.paragraphs:
            text += para.text + "\n"
    elif ext == "txt":
        with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
            text = f.read()

    # replace special unicode characters before saving
    text = clean_unicode(text)
    return text.strip()


def clean_unicode(text):
    # replace common special characters with their ascii equivalent
    replacements = {
        '\u2018': "'", '\u2019': "'",  # curly single quotes
        '\u201c': '"', '\u201d': '"',  # curly double quotes
        '\u2013': '-', '\u2014': '-',  # en dash, em dash
        '\u2026': '...', '\u00a0': ' ', # ellipsis, non-breaking space
        '\u00e9': 'e', '\u00e8': 'e',  # accented e
        '\u00e0': 'a', '\u00e1': 'a',  # accented a
        '\u00f3': 'o', '\u00f4': 'o',  # accented o
        '\u00fa': 'u', '\u00fc': 'u',  # accented u
        '\u00ed': 'i', '\u00ee': 'i',  # accented i
        '\u00f1': 'n',                  # n with tilde
        '\u00e7': 'c',                  # c with cedilla
        '\u2022': ' ', '\u00b7': ' ',  # bullet points
    }
    for char, replacement in replacements.items():
        text = text.replace(char, replacement)

    # remove any remaining non-ascii characters
    text = text.encode("ascii", "ignore").decode("ascii")
    return text


def save_text(text):
    text = clean_unicode(text)
    output_path = os.path.join(UPLOAD_FOLDER, "user_text.txt")
    with open(output_path, "w", encoding="utf-8") as f:
        f.write(text)


def preprocess_text(text):
    result = ""
    in_bracket = False
    for c in text:
        if c == '[':
            in_bracket = True
        elif c == ']':
            in_bracket = False
        elif not in_bracket:
            result += c

    # step 2 & 3: lowercase + keep only letters/digits/spaces
    cleaned = ""
    for c in result:
        c = c.lower()
        if c.isalpha() or c.isdigit():
            cleaned += c
        else:
            cleaned += ' '

    # step 4: collapse multiple spaces
    cleaned = re.sub(r' +', ' ', cleaned).strip()

    return cleaned


def extract_queries(text, num_queries=5):
    # remove bracket markers like [24] [j] from text before extracting queries
    text = re.sub(r'\[[^\]]*\]', ' ', text)
    text = re.sub(r' +', ' ', text).strip()

    sentences = text.replace("\n", " ").split(".")
    sentences = [s.strip() for s in sentences if len(s.strip().split()) >= 6]

    if not sentences:
        return []

    queries = []
    seen = set()

    # spread queries evenly across text
    # e.g. 5 queries from 20 sentences → pick sentences 0, 4, 8, 12, 16
    step = max(1, len(sentences) // num_queries)

    for i in range(0, len(sentences), step):
        q = sentences[i][:120]
        if q not in seen:
            seen.add(q)
            queries.append(q)
        if len(queries) >= num_queries:
            break

    # if still not enough, fill from remaining sentences
    if len(queries) < num_queries:
        for s in sentences:
            q = s[:120]
            if q not in seen:
                seen.add(q)
                queries.append(q)
            if len(queries) >= num_queries:
                break

    return queries[:num_queries]


def fetch_wikipedia(title):
    try:
        clean_title = title.replace("_", " ")
        page = wiki.page(clean_title)
        if not page.exists():
            return ""

        # get full text
        text = page.text

        # remove section titles by removing lines that are
        # exact matches to section names in the page
        def remove_section_titles(page, text):
            for section in page.sections:
                # remove exact section title from text
                text = text.replace('\n' + section.title + '\n', ' ')
                text = text.replace(section.title + '\n', ' ')
                # recursively handle subsections
                for subsection in section.sections:
                    text = text.replace('\n' + subsection.title + '\n', ' ')
                    text = text.replace(subsection.title + '\n', ' ')
            return text

        text = remove_section_titles(page, text)
        text = preprocess_text(text)

        print(f"Wikipedia '{clean_title}' returned {len(text.split())} words")
        return text

    except Exception as e:
        print(f"Wikipedia fetch failed: {e}")
        return ""


def search_wikipedia_by_phrase(phrase):
    """Search Wikipedia for articles containing exact phrase"""
    titles = []
    try:
        # use Wikipedia search API with the phrase
        search_url = f"https://en.wikipedia.org/w/api.php?action=query&list=search&srsearch={requests.utils.quote(phrase)}&srlimit=5&format=json"
        response = requests.get(search_url, timeout=8, headers={"User-Agent": "PlagDetect/1.0"})
        if response.status_code == 200:
            results = response.json().get("query", {}).get("search", [])
            for result in results:
                titles.append(result.get("title", ""))
    except Exception as e:
        print(f"Wikipedia phrase search failed: {e}")
    return titles


def fetch_website(url):
    try:
        response = requests.get(url, timeout=10, headers=HEADERS)

        if response.status_code != 200:
            print(f"Failed with status {response.status_code}: {url}")
            return ""

        response.encoding = response.apparent_encoding

        soup = BeautifulSoup(response.text, "html.parser")
        for tag in soup(["script", "style", "nav", "footer", "header", "aside",
                         "figure", "figcaption", "form", "button", "iframe"]):
            tag.decompose()

        # grab only article body, not full page
        main = (
            soup.find("article") or
            soup.find(attrs={"class": re.compile(r'article|story|content|post|body', re.I)}) or
            soup.find(attrs={"id": re.compile(r'article|story|content|post|body', re.I)}) or
            soup.find("main") or
            soup
        )

        page_text = main.get_text(separator=" ")

        meta_desc = soup.find("meta", attrs={"name": "description"})
        meta_key  = soup.find("meta", attrs={"name": "keywords"})
        if meta_desc and meta_desc.get("content"):
            page_text += " " + meta_desc["content"]
        if meta_key and meta_key.get("content"):
            page_text += " " + meta_key["content"]

        return preprocess_text(page_text)

    except Exception as e:
        print(f"Failed to fetch {url}: {e}")
        return ""

def search_and_fetch(text):
    fetched_files = []
    fetched_sources = []
    queries = extract_queries(text, num_queries=8)

    if not queries:
        return fetched_files, fetched_sources

    print(f"Searching with {len(queries)} queries...")

    file_index = 1
    seen_titles = set()
    seen_urls = set()
    ddgs = DDGS()

    for query in queries:
        print(f"\nQuery: {query[:60]}...")

        # Search Wikipedia with exact phrase — finds actual source article
        for title in search_wikipedia_by_phrase(query):
            if title in seen_titles:
                continue
            seen_titles.add(title)

            page_text = fetch_wikipedia(title)
            if len(page_text.split()) < 10:
                continue

            filepath = f"dataset/search_{file_index}.txt"
            with open(filepath, "w", encoding="utf-8", errors="ignore") as f:
                f.write(page_text)

            fetched_files.append(filepath)
            fetched_sources.append({
                "url": f"https://en.wikipedia.org/wiki/{title.replace(' ', '_')}",
                "title": title,
                "file": filepath
            })
            print(f"Saved Wikipedia '{title}' ({len(page_text.split())} words)")
            file_index += 1

        # DuckDuckGo search
        try:
            exact_results = list(ddgs.text(f'"{query[:80]}"', max_results=3))
            broad_results = list(ddgs.text(query[:100], max_results=3))

            for result in exact_results + broad_results:
                url = result.get("href", "")
                if not url or url in seen_urls:
                    continue
                seen_urls.add(url)

                if "wikipedia.org/wiki/" in url:
                    title = url.split("/wiki/")[1].split("#")[0].replace("_", " ")
                    if title in seen_titles:
                        continue
                    seen_titles.add(title)
                    page_text = fetch_wikipedia(title)
                    source_title = title
                else:
                    page_text = fetch_website(url)
                    source_title = result.get("title", url)

                if len(page_text.split()) < 10:
                    continue

                filepath = f"dataset/search_{file_index}.txt"
                with open(filepath, "w", encoding="utf-8", errors="ignore") as f:
                    f.write(page_text)

                fetched_files.append(filepath)
                fetched_sources.append({
                    "url": url,
                    "title": source_title,
                    "file": filepath
                })
                print(f"Saved: {filepath} ({len(page_text.split())} words)")
                file_index += 1

        except Exception as e:
            print(f"DDG search failed: {e}")

    print(f"\nTotal pages fetched: {len(fetched_files)}")
    return fetched_files, fetched_sources


def extract_score(output):
    for line in output.splitlines():
        if "Similarity Score" in line:
            try:
                return int(line.split(":")[1].replace("%", "").strip())
            except:
                return 0
    return 0


def extract_time(output):
    for line in output.splitlines():
        if "Execution Time" in line:
            try:
                return int(line.split(":")[1].replace("ms", "").strip())
            except:
                return 0
    return 0


def extract_score_per_file(output):
    scores = {}
    for line in output.splitlines():
        if "vs " in line and " : " in line and "%" in line:
            try:
                parts = line.strip().split(" : ")
                filepath = parts[0].replace("vs ", "").strip()
                score = int(parts[1].replace("%", "").strip())
                scores[filepath] = score
            except:
                pass
    return scores


def delete_user_files():
    user_files = [
        "uploads/user_text.txt",
        "uploads/cleaned_text.txt",
        "uploads/user_file.pdf",
        "uploads/user_file.doc",
        "uploads/user_file.docx",
        "uploads/user_file.txt"
    ]
    for filepath in user_files:
        try:
            if os.path.exists(filepath):
                os.remove(filepath)
                print(f"Deleted user file: {filepath}")
        except:
            pass


def run_engine(user_text):

    # Step 1: preprocess
    subprocess.run(["BackendAlgo\\preprocess.exe"])

    # Step 2: search and fetch
    fetched_files, fetched_sources = search_and_fetch(user_text)

    if not fetched_files:
        delete_user_files()
        return 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, []

    # Step 3: naive
    naive_result = subprocess.run(["BackendAlgo\\naive.exe"], capture_output=True, text=True)
    naive_score = extract_score(naive_result.stdout)
    naive_time  = extract_time(naive_result.stdout)
    naive_per_file = extract_score_per_file(naive_result.stdout)
    print(f"Naive Score: {naive_score}% | Time: {naive_time}ms")

    # Step 4: kmp
    kmp_result = subprocess.run(["BackendAlgo\\kmp.exe"], capture_output=True, text=True)
    kmp_score = extract_score(kmp_result.stdout)
    kmp_time  = extract_time(kmp_result.stdout)
    print(f"KMP Score: {kmp_score}% | Time: {kmp_time}ms")

    # Step 5: rabin-karp
    rk_result = subprocess.run(["BackendAlgo\\rabin_karp.exe"], capture_output=True, text=True)
    rk_score = extract_score(rk_result.stdout)
    rk_time  = extract_time(rk_result.stdout)
    print(f"Rabin-Karp Score: {rk_score}% | Time: {rk_time}ms")

    # Step 6: shingling
    sh_result = subprocess.run(["BackendAlgo\\shingling.exe"], capture_output=True, text=True)
    sh_score = extract_score(sh_result.stdout)
    sh_time  = extract_time(sh_result.stdout)
    print(f"Shingling Score: {sh_score}% | Time: {sh_time}ms")

    # Step 7: lcs
    lcs_result = subprocess.run(["BackendAlgo\\lcs.exe"], capture_output=True, text=True)
    lcs_score = extract_score(lcs_result.stdout)
    lcs_time  = extract_time(lcs_result.stdout)
    print(f"LCS Score: {lcs_score}% | Time: {lcs_time}ms")

    # Step 8: find matched sources
    matched_sources = []
    for source in fetched_sources:
        filepath = source["file"].replace("/", "\\")
        score = naive_per_file.get(filepath, 0)
        if score > 0:
            matched_sources.append({
                "url": source["url"],
                "title": source["title"],
                "score": score
            })
    matched_sources.sort(key=lambda x: x["score"], reverse=True)

    # Step 9: delete fetched files
    for filepath in fetched_files:
        try:
            os.remove(filepath)
            print(f"Deleted: {filepath}")
        except:
            pass

    # Step 10: delete user files
    delete_user_files()

    # Step 11: final score = highest score across all 5 algorithms
    final_score = max(naive_score, kmp_score, rk_score, sh_score, lcs_score)
    print(f"Final Score: {final_score}%")

    return naive_score, kmp_score, rk_score, sh_score, lcs_score, final_score, naive_time, kmp_time, rk_time, sh_time, lcs_time, matched_sources


@app.route("/upload", methods=["POST"])
def upload_file():

    if "file" in request.files and request.files["file"].filename != "":
        file = request.files["file"]
        if not allowed_file(file.filename):
            return jsonify({"error": "Unsupported file type. Use PDF, DOC, DOCX, or TXT."}), 400
        ext = file.filename.rsplit(".", 1)[1].lower()
        saved_path = os.path.join(UPLOAD_FOLDER, "user_file." + ext)
        file.save(saved_path)
        text = extract_text(saved_path, ext)
        if not text:
            return jsonify({"error": "Could not extract text from the file."}), 400
        save_text(text)

    elif "text" in request.form and request.form["text"].strip() != "":
        text = request.form["text"].strip()
        save_text(text)

    else:
        return jsonify({"error": "No file or text provided."}), 400

    naive_score, kmp_score, rk_score, sh_score, lcs_score, final_score, naive_time, kmp_time, rk_time, sh_time, lcs_time, matched_sources = run_engine(text)

    return jsonify({
        "similarity": final_score,
        "naive_score": naive_score,
        "kmp_score": kmp_score,
        "rk_score": rk_score,
        "sh_score": sh_score,
        "lcs_score": lcs_score,
        "naive_time": naive_time,
        "kmp_time": kmp_time,
        "rk_time": rk_time,
        "sh_time": sh_time,
        "lcs_time": lcs_time,
        "matched_sources": matched_sources,
        "message": "Done"
    })


if __name__ == "__main__":
    app.run(debug=True, port=5000)