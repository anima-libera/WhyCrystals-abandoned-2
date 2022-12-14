
""" Content embedding via the generation of a C source file.

The header file at `get_embedded_header_file_path()` is allowed special variable declarations
via the magic macro `EMBEDDED`. These declarations are in fact requests to the buildsystem
to embed the content of some files (specified in the macro calls) into the executable,
as definitions of the declared variables. It is done by generating the C file at
`get_embedded_c_file_path()` with definitions of these variables initialized to the
content of the specified files.
"""

import os
import re
import enum
from buildsystem.structure import SRC_DIR, get_embedded_header_file_path, get_embedded_c_file_path
from buildsystem.utils import *

class EscapeMode(enum.Enum):
	TEXT = enum.auto() # Escape the content as a string literal.
	BINARY = enum.auto() # Escape the content as a byte array.
	SIZE = enum.auto() # Escape the content as the size of its byte array.

	@classmethod
	def from_str(cls, escape_mode_str: str) -> "EscapeMode":
		return {
			"TEXT": EscapeMode.TEXT,
			"BINARY": EscapeMode.BINARY,
			"SIZE": EscapeMode.SIZE
		}[escape_mode_str]

class FileToEmbed:
	def __init__(self, file_path: Path, escape_mode: EscapeMode, variable_declaration: str):
		self.file_path = file_path
		self.escape_mode = escape_mode
		self.variable_declaration = variable_declaration

	def get_espaced_content(self) -> str:
		""" Returns the C++ literal corresponding to the content of the file
		escaped according to the escape mode. """
		try:
			def escape_as_string(string):
				return "\"" + string.translate({
					ord("\""): "\\\"", ord("\\"): "\\\\",
					ord("\n"): "\\n", ord("\t"): "\\t"}) + "\""
			opening_mode, escape_function = {
				EscapeMode.TEXT: (
					"rt", escape_as_string),
				EscapeMode.BINARY: (
					"rb", lambda bytes: "{" + ", ".join([hex(b) for b in bytes]) + "}"),
				EscapeMode.SIZE: (
					"rb", lambda bytes: str(len(bytes))),
			}[self.escape_mode]
			with open(self.file_path, opening_mode) as file:
				return escape_function(file.read())
		except FileNotFoundError as error:
			print_error("Embeding file error",
				"The embedded content generator could not find the file " +
				f"\"{self.file_path}\" used in an EMBEDDED macro in the " +
				f"\"{get_embedded_header_file_path()}\" header file.")
		raise error

def generate_c(files_to_embed: List[FileToEmbed]) -> str:
	generated_c: List[str] = []
	generated_c.append("")
	generated_c.append("/* This file may be overwritten or removed by the buildsystem.")
	generated_c.append(f" * Do not modify, see \"{get_embedded_header_file_path()}\" " +
		"or \"buildsystem/embed.py\" instead. */")
	generated_c.append("")
	for file_to_embed in files_to_embed:
		what = "Size in bytes" if file_to_embed.escape_mode == EscapeMode.SIZE else "Content"
		generated_c.append("")
		generated_c.append(f"/* {what} of \"{file_to_embed.file_path}\". */")
		variable_declaration = file_to_embed.variable_declaration
		escaped_content = file_to_embed.get_espaced_content()
		generated_c.append(f"{variable_declaration} = {escaped_content};")
	generated_c.append("")
	return "\n".join(generated_c)

def handle_embedding() -> None:
	""" Makes sure that the c file that should contain the escaped content of the files to embed
	is synchronized with the said files. """

	embedded_c_file_path = get_embedded_c_file_path()
	# Comparing the dates of the header and of the files to embed to the date of the generated C
	# will tell if changes in that C file (new embedding) are required or not.
	# If none of these is more recent that the C file, then nothing has changed here.
	if os.path.isfile(embedded_c_file_path):
		embedded_c_date = os.path.getmtime(embedded_c_file_path)
	else:
		embedded_c_date = 0.0

	embedded_header_file_path = get_embedded_header_file_path()
	if not os.path.isfile(embedded_header_file_path):
		print_error("Embeding file warning",
			"The embedded content generator could not find the header file " +
			f"\"{embedded_header_file_path}\" and thus could not embed any content.")
		return

	files_that_require_embedding: List[Path] = []
	# Changes in the header can be about variable declarations, removed content, etc.
	if embedded_c_date < os.path.getmtime(embedded_header_file_path):
		files_that_require_embedding.append(embedded_header_file_path)
	files_to_embed: List[FileToEmbed] = []
	with open(embedded_header_file_path, "r") as embedded_header_file:
		EMBEDDED_RE = (
			r"EMBEDDED\s*\(\s*\"([^\"]+)\"\s*,\s*(TEXT|BINARY|SIZE)\s*\)\s*([^\s][^;]+[^\s])\s*;")
		for match in re.finditer(EMBEDDED_RE, embedded_header_file.read()):
			raw_file_path = match.group(1)
			# The path menstionned in the macro uses should start after the `SRC_DIR` part.
			file_path = os.path.join(SRC_DIR, raw_file_path)
			if embedded_c_date < os.path.getmtime(file_path):
				files_that_require_embedding.append(file_path)
			escape_mode = EscapeMode.from_str(match.group(2))
			variable_declaration = match.group(3)
			files_to_embed.append(FileToEmbed(file_path, escape_mode, variable_declaration))

	if not files_that_require_embedding:
		print_verbose("No embedding required.")
	else:
		print_verbose("Embedding required due to the following files:")
		for file_path in files_that_require_embedding:
			print_verbose(f"- \"{file_path}\"")
		with open(embedded_c_file_path, "w") as embedded_c_file:
			print_verbose(f"Generating \"{embedded_c_file_path}\" " +
				"with the escaped content to embed.")
			embedded_c_file.write(generate_c(files_to_embed))
