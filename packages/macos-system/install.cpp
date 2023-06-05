// install.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <vector>
#include <optional>
#include <filesystem>
#include <string_view>

[[noreturn]] static void error_and_exit(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	exit(1);
}

static bool starts_with(std::string_view a, std::string_view b)
{
	return a.substr(0, b.size()) == b;
}

static gid_t parse_group(const std::string& group_str);
static uid_t parse_user(const std::string& user_str);
static mode_t parse_mode(const char *s, mode_t current_mode);
static mode_t parse_mode(const std::string& mode_str);

static std::string parse_string(const std::string& s)
{
	return s;
}

template <typename T>
static T parse_thing(
	const std::vector<std::string>& args,
	size_t& idx,
	std::string_view arg,
	T (*fn)(const std::string&),
	bool is_short = false)
{
	// printf("arg = '%s'\n", arg.data());
	if(starts_with(arg, "=") && arg.size() > 1)
	{
		return fn(std::string(arg.substr(1)));
	}
	else if(is_short && not arg.empty())
	{
		return fn(std::string(arg));
	}
	else if(arg.empty())
	{
		if(idx + 1 < args.size())
			return fn(std::string(args[++idx]));
		else
			error_and_exit("expected argument (arg was '%.*s')\n", (int) arg.size(), arg.data());
	}
	else
	{
		error_and_exit("expected argument (arg was '%.*s')\n", (int) arg.size(), arg.data());
	}
}

namespace stdfs = std::filesystem;

static bool mkdir_leading(const stdfs::path& path, mode_t mode);
static void print_usage();

static bool g_opt_verbose = false;
static bool g_opt_preserve_timestamps = false;

static std::optional<gid_t> g_opt_group {};
static std::optional<mode_t> g_opt_mode {};
static std::optional<uid_t> g_opt_owner {};


static bool set_mode_and_perms(const stdfs::path& path);
static std::error_code set_timestamps(const stdfs::path& src, const stdfs::path& dst);

static std::error_code copy_recursively(const stdfs::path& source, const stdfs::path& dest);


int main(int argc, char** argv)
{
	std::vector<std::string> args;
	for(int i = 1; i < argc; i++)
		args.emplace_back(argv[i]);

	bool opt_compare = false;
	bool opt_all_dirs = false;
	bool opt_create_leading_dirs = false;
	bool opt_no_target_directory = false;

	std::optional<std::string> opt_target_dir {};

	bool no_more_args = false;
	std::vector<std::string> paths {};
	for(size_t idx = 0; idx < args.size(); idx++)
	{
		auto arg = std::string_view(args[idx]);

		if(arg.empty())
		{
			continue;
		}
		else if(arg == "--")
		{
			no_more_args = true;
			continue;
		}

		if(not no_more_args && arg[0] == '-' && not starts_with(arg, "--"))
		{
			bool stop = false;
			for(size_t i = 1; not stop && i < arg.size(); i++)
			{
				switch(arg[i])
				{
				case 'v':   g_opt_verbose = true;               break;
				case 'p':   g_opt_preserve_timestamps = true;   break;
				case 'C':   opt_compare = true;                 break;
				case 'D':   opt_create_leading_dirs = true;     break;
				case 'T':   opt_no_target_directory = true;     break;
				case 'd':   opt_all_dirs = true;                break;

				case 'c':   // ignored
				case 's':   // strip: do nothing
					break;

				case 'm':
					g_opt_mode = parse_thing(args, idx, arg.substr(1 + i), parse_mode, /* is_short: */ true);
					stop = true;
					break;

				case 'g':
					g_opt_group = parse_thing(args, idx, arg.substr(1 + i), parse_group, /* is_short: */ true);
					stop = true;
					break;

				case 'o':
					g_opt_owner = parse_thing(args, idx, arg.substr(1 + i), parse_user, /* is_short: */ true);
					stop = true;
					break;

				case 't':
					opt_target_dir = parse_thing(args, idx, arg.substr(1 + i), parse_string, /* is_short: */ true);
					stop = true;
					break;

				case 'h':
					print_usage();
					exit(0);

				default:
					error_and_exit("unsupported argument '-%c'\n", arg[i]);
					break;
				}
			}
		}
		else if(not no_more_args && starts_with(arg, "--"))
		{
			if(arg == "--help")
			{
				print_usage();
				exit(0);
			}
			else if(arg == "--compare")
			{
				opt_compare = true;
			}
			else if(arg == "--strip")
			{
				// do nothing
			}
			else if(arg == "--verbose")
			{
				g_opt_verbose = true;
			}
			else if(arg == "--no-target-directory")
			{
				opt_no_target_directory = true;
			}
			else if(arg == "--directory")
			{
				opt_all_dirs = true;
			}
			else if(arg == "--preserve-timestamps")
			{
				g_opt_preserve_timestamps = true;
			}
			else if(starts_with(arg, "--group"))
			{
				arg.remove_prefix(strlen("--group"));
				g_opt_group = parse_thing(args, idx, arg, parse_group);
			}
			else if(starts_with(arg, "--owner"))
			{
				arg.remove_prefix(strlen("--owner"));
				g_opt_owner = parse_thing(args, idx, arg, parse_user);
			}
			else if(starts_with(arg, "--mode"))
			{
				arg.remove_prefix(strlen("--mode"));
				g_opt_mode = parse_thing(args, idx, arg, parse_mode);
			}
			else if(starts_with(arg, "--target-directory"))
			{
				arg.remove_prefix(strlen("--target-directory"));
				opt_target_dir = parse_thing(args, idx, arg, parse_string);
			}
			else
			{
				error_and_exit("unsupported option '%.*s'\n", (int) arg.size(), arg.data());
			}
		}
		else
		{
			paths.push_back(std::move(args[idx]));
		}
	}

	(void) opt_compare;

	if(opt_target_dir.has_value() && opt_all_dirs)
		error_and_exit("cannot use -t with -d\n");

	if(opt_target_dir.has_value() && opt_no_target_directory)
		error_and_exit("cannot use -t with -T\n");

	if(paths.empty() || not (opt_all_dirs || opt_target_dir.has_value() || paths.size() >= 2))
		error_and_exit("expected at least two arguments, got %zu\n", paths.size());

	// just for now
	// g_opt_verbose = true;

	auto copy_files = [&](const stdfs::path& src, stdfs::path dst) -> bool {
		auto ec = copy_recursively(src, dst);
		if(ec)
		{
			fprintf(stderr, "failed to copy '%s' -> '%s': %s (%d)\n",
				src.c_str(), dst.c_str(), ec.message().c_str(), ec.value());
			return false;
		}

		return true;
	};


	if(opt_all_dirs)
	{
		bool all_ok = true;
		for(const auto& path : paths)
			all_ok &= (mkdir_leading(path, 0755) && set_mode_and_perms(path));

		return all_ok ? 0 : 1;
	}
	else if(opt_no_target_directory)
	{
		if(paths.size() != 2)
			error_and_exit("expected exactly 2 paths for '-T'\n");

		auto src = stdfs::path(paths[0]);
		auto dst = stdfs::path(paths[1]);

		bool ok = true;
		if(opt_create_leading_dirs)
			ok &= mkdir_leading(dst.parent_path(), 0755);

		ok &= copy_files(src, dst);
		ok &= set_mode_and_perms(dst);
		ok &= (set_timestamps(src, dst) == std::error_code{});

		return ok ? 0 : 1;
	}
	else
	{
		auto dst_dir = [&]() -> stdfs::path {
			if(opt_target_dir.has_value())
				return *opt_target_dir;

			auto ret = paths.back();
			paths.erase(paths.end() - 1);
			return ret;
		}();

		bool all_ok = true;
		if(opt_create_leading_dirs)
			all_ok &= mkdir_leading(opt_target_dir.has_value() ? dst_dir : dst_dir.parent_path(), 0755);

		for(const auto& src : paths)
			all_ok &= copy_files(src, dst_dir);

		return all_ok ? 0 : 1;
	}
}


static stdfs::path read_symlink_recursively(const stdfs::path& path)
{
	std::error_code ec {};
	auto ret = stdfs::canonical(path, ec);
	if(ec)
	{
		fprintf(stderr, "failed to resolve path '%s': %s (%d)\n", path.c_str(), ec.message().c_str(), ec.value());
		exit(1);
	}

	return ret;
}



static std::error_code copy_recursively(const stdfs::path& source, const stdfs::path& dest)
{
	if(stdfs::is_directory(source) && (stdfs::is_regular_file(dest) || stdfs::is_symlink(dest)))
		return std::make_error_code(std::errc::is_a_directory);

	if(not stdfs::exists(source))
	{
		fprintf(stderr, "'%s' does not exist\n", source.c_str());
		return std::make_error_code(std::errc::no_such_file_or_directory);
	}

	if(g_opt_verbose)
		printf("copy '%s' -> '%s'\n", source.c_str(), dest.c_str());

#if 0
	if(stdfs::is_symlink(source))
	{
		auto dst_file = dest;
		if(stdfs::exists(dst_file))
		{
			if(stdfs::is_directory(dst_file))
				dst_file /= source.filename();

			else if(not(stdfs::is_regular_file(dst_file) || stdfs::is_symlink(dst_file)))
				return std::make_error_code(std::errc::is_a_directory);

			unlink(dst_file.c_str());
		}

		std::error_code ec {};
		stdfs::copy_symlink(source, dst_file, ec);
		if(ec)
			return ec;

		if(auto x = set_timestamps(source, dst_file); x)
			return x;

		if(not set_mode_and_perms(dst_file))
			return std::make_error_code(static_cast<std::errc>(errno));

		return {};
	}
	else
#endif

	auto is_streaming_source = [](const stdfs::path& p) -> bool {
		return stdfs::is_fifo(p) || stdfs::is_character_file(p);
	};

	// some install scripts like to use /dev/stdin as the source...
	// note that we check for this first, because /dev/stdin might be a symlink to /dev/fd/0
	if(is_streaming_source(source) || (stdfs::is_symlink(source) && is_streaming_source(read_symlink_recursively(source))))
	{
		std::error_code ec {};
		auto dst_file = (stdfs::exists(dest) && stdfs::is_directory(dest))
			? dest / source.filename()
			: dest;

		int dst_fd = open(dst_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if(dst_fd < 0)
		{
			fprintf(stderr, "could not create destination: %s (%d)\n", strerror(errno), errno);
			return std::error_code(errno, std::generic_category());
		}

		int src_fd = open(source.c_str(), O_RDONLY);
		if(src_fd < 0)
		{
			fprintf(stderr, "could not open source file: %s (%d)\n", strerror(errno), errno);
			return std::error_code(errno, std::generic_category());
		}

		uint8_t buf[1024] {};
		while(true)
		{
			auto did_read = read(src_fd, &buf[0], 1024);
			if(did_read == 0)
				break;

			if(did_read < 0)
			{
				fprintf(stderr, "could not read from source: %s (%d)\n", strerror(errno), errno);
				return std::error_code(errno, std::generic_category());
			}

			for(ssize_t k = 0; k < did_read;)
			{
				auto did_write = write(dst_fd, &buf[k], did_read - k);
				if(did_write <= 0)
				{
					fprintf(stderr, "could not write to destination: %s (%d)\n", strerror(errno), errno);
					return std::error_code(errno, std::generic_category());
				}

				k += did_write;
			}
		}

		if(not set_mode_and_perms(dst_file))
			return std::make_error_code(static_cast<std::errc>(errno));

		return {};
	}
	else if(stdfs::is_regular_file(source) || stdfs::is_symlink(source))
	{
		std::error_code ec {};
		auto dst_file = (stdfs::exists(dest) && stdfs::is_directory(dest))
			? dest / source.filename()
			: dest;

		stdfs::copy_file(source, dst_file, stdfs::copy_options::overwrite_existing, ec);
		if(ec)
			return ec;

		if(auto x = set_timestamps(source, dst_file); x)
			return x;

		if(not set_mode_and_perms(dst_file))
			return std::make_error_code(static_cast<std::errc>(errno));

		return {};
	}
	else if(stdfs::is_directory(source))
	{
		std::error_code ec {};
		if(not stdfs::exists(dest))
		{
			if(not stdfs::create_directory(dest, source, ec))
				return ec;
		}

		ec.clear();
		for(const auto& x : stdfs::directory_iterator(source, ec))
		{
			if(auto ec = copy_recursively(x.path(), dest / x.path().filename()); ec)
				return ec;
		}

		return {};
	}
	else
	{
		fprintf(stderr, "unknown file type of '%s'\n", source.c_str());
		return std::make_error_code(std::errc::invalid_argument);
	}
}



static bool set_mode_and_perms(const stdfs::path& path)
{
	bool ok = true;

	if(g_opt_mode.has_value())
	{
		if(chmod(path.c_str(), *g_opt_mode) < 0)
			ok = false, fprintf(stderr, "failed to set mode of '%s': %s (%d)\n", path.c_str(), strerror(errno), errno);
		else if(g_opt_verbose)
			printf("set mode of '%s' = %o\n", path.c_str(), *g_opt_mode);
	}

	auto set_mode = [&](const stdfs::path& path, uid_t uid, gid_t gid) {
		if(lchown(path.c_str(), uid, gid) < 0)
			ok = false, fprintf(stderr, "failed to change ownership of '%s': %s (%d)\n", path.c_str(), strerror(errno), errno);
		else if(g_opt_verbose)
			printf("change ownership of '%s' = %u:%u\n", path.c_str(), uid, gid);
	};

	if(g_opt_group.has_value() && g_opt_owner.has_value())
		set_mode(path, *g_opt_owner, *g_opt_group);
	else if(g_opt_group.has_value())
		set_mode(path, getuid(), *g_opt_group);
	else if(g_opt_owner.has_value())
		set_mode(path, *g_opt_owner, getgid());

	return ok;
}

static std::error_code set_timestamps(const stdfs::path& src, const stdfs::path& dst)
{
	if(not g_opt_preserve_timestamps)
		return {};

	std::error_code ec {};
	auto t = stdfs::last_write_time(src, ec);
	if(ec)
	{
		fprintf(stderr, "failed to get modification time of '%s': %s (%d)\n",
			src.c_str(), ec.message().c_str(), ec.value());
		return ec;
	}

	stdfs::last_write_time(dst, t, ec);
	if(ec)
	{
		fprintf(stderr, "failed to set modification time of '%s': %s (%d)\n",
			dst.c_str(), ec.message().c_str(), ec.value());
		return ec;
	}

	return {};
}




static bool mkdir_leading(const stdfs::path& path, mode_t mode)
{
	stdfs::path current {};

	for(auto dir : path)
	{
		if(dir.empty())
			continue;

		std::error_code ec {};
		if(not stdfs::create_directories(current / dir, ec) && ec)
		{
			fprintf(stderr, "failed to create directories ('%s') for '%s': %s (%d)\n",
				(current / dir).c_str(), path.c_str(), ec.message().c_str(), ec.value());
			return false;
		}

		current /= dir;
	}
	return true;
}

static void print_usage()
{
	auto s = ""
"Usage: install [OPTION]... [-T] SOURCE DEST"                                                       "\n"
"  or:  install [OPTION]... SOURCE... DIRECTORY"                                                    "\n"
"  or:  install [OPTION]... -t DIRECTORY SOURCE..."                                                 "\n"
"  or:  install [OPTION]... -d DIRECTORY..."                                                        "\n"
""                                                                                                  "\n"
"In the first three forms, copy SOURCE to DEST or multiple SOURCE(s) to"                            "\n"
"the existing DIRECTORY, while setting permission modes and owner/group."                           "\n"
"In the 4th form, create all components of the given DIRECTORY(ies)."                               "\n"
""                                                                                                  "\n"
"Mandatory arguments to long options are mandatory for short options too."                          "\n"
"  -C, --compare                         ignored"                                                   "\n"
"  -d, --directory                       treat all arguments as directory names; create all"        "\n"
"                                          components of the specified directories"                 "\n"
"  -D                                    create all leading components of DEST except the last,"    "\n"
"                                          or all components of --target-directory,"                "\n"
"                                          then copy SOURCE to DEST"                                "\n"
"  -g, --group=GROUP                     set group ownership, instead of process' current group"    "\n"
"  -m, --mode=MODE                       set permission mode (as in chmod), instead of rwxr-xr-x"   "\n"
"  -o, --owner=OWNER                     set ownership (super-user only)"                           "\n"
"  -p, --preserve-timestamps             apply access/modification times of SOURCE files"           "\n"
"                                          to corresponding destination files"                      "\n"
"  -s, --strip                           ignored"                                                   "\n"
"  -t, --target-directory=DIRECTORY      copy all SOURCE arguments into DIRECTORY"                  "\n"
"  -T, --no-target-directory             treat DEST as a normal file"                               "\n"
"  -v, --verbose                         print the name of each directory as it is created"         "\n"
"  -h, --help                            display this help and exit"                                "\n";
	printf("%s\n", s);
}






static gid_t parse_group(const std::string& group_str)
{
	struct group* grp = nullptr;
	if(grp = getgrnam(group_str.c_str()); grp == nullptr)
	{
		// try group id
		try {
			auto gid = (gid_t) std::stoul(group_str);
			grp = getgrgid(gid);
			if(grp == nullptr)
				throw 0;
		} catch(...) {
			error_and_exit("no group named '%s'\n", group_str.c_str());
		}
	}

	assert(grp != nullptr);
	return grp->gr_gid;
}

static uid_t parse_user(const std::string& user_str)
{
	struct passwd* pw = nullptr;
	if(pw = getpwnam(user_str.c_str()); pw == nullptr)
	{
		// try user id
		try {
			auto uid = (uid_t) std::stoul(user_str);
			pw = getpwuid(uid);
			if(pw == nullptr)
				throw 0;
		} catch(...) {
			error_and_exit("no user named '%s'\n", user_str.c_str());
		}
	}

	assert(pw != nullptr);
	return pw->pw_uid;
}

static mode_t parse_mode(const std::string& mode_str)
{
	return parse_mode(mode_str.c_str(), 0755);
}


/*
 * parse_mode implementation for busybox
 *
 * Copyright (C) 2003  Manuel Novoa III  <mjn3@codepoet.org>
 *
 * Licensed under GPLv2 or later, see file LICENSE in this source tree.
 */
#define FILEMODEBITS ((mode_t) (S_ISUID | S_ISGID | S_ISVTX | S_IRWXU | S_IRWXG | S_IRWXO))
static mode_t parse_mode(const char *s, mode_t current_mode)
{
	/* should be mode_t really, but in all Unixes these constants fit into uint16 */
	static const mode_t who_mask[] = {
		S_ISUID | S_ISGID | S_ISVTX | S_IRWXU | S_IRWXG | S_IRWXO, /* a */
		S_ISUID | S_IRWXU,           /* u */
		S_ISGID | S_IRWXG,           /* g */
		S_IRWXO                      /* o */
	};
	static const mode_t perm_mask[] = {
		S_IRUSR | S_IRGRP | S_IROTH, /* r */
		S_IWUSR | S_IWGRP | S_IWOTH, /* w */
		S_IXUSR | S_IXGRP | S_IXOTH, /* x */
		S_IXUSR | S_IXGRP | S_IXOTH, /* X -- special -- see below */
		S_ISUID | S_ISGID,           /* s */
		S_ISVTX                      /* t */
	};
	static const char who_chars[] = "augo";
	static const char perm_chars[] = "rwxXst";

	const char *p;
	mode_t wholist;
	mode_t permlist;
	mode_t new_mode;
	char op;

	const char* old_s = s;
	if ((unsigned char)(*s - '0') < 8) {
		unsigned long tmp;
		char *e;

		tmp = strtoul(s, &e, 8);
		if (*e || (tmp > 07777U)) { /* Check range and trailing chars. */
			error_and_exit("invalid mode '%s'\n", s);
		}

		return (mode_t) tmp;
	}

	new_mode = current_mode;

	/* Note: we allow empty clauses, and hence empty modes.
	 * We treat an empty mode as no change to perms. */

	while (*s) {  /* Process clauses. */
		if (*s == ',') {  /* We allow empty clauses. */
			++s;
			continue;
		}

		/* Get a wholist. */
		wholist = 0;
 WHO_LIST:
		p = who_chars;
		do {
			if (*p == *s) {
				wholist |= who_mask[(int)(p-who_chars)];
				if (!*++s) {
					error_and_exit("invalid mode '%s'\n", old_s);
				}
				goto WHO_LIST;
			}
		} while (*++p);

		do {    /* Process action list. */
			if ((*s != '+') && (*s != '-')) {
				if (*s != '=') {
					error_and_exit("invalid mode '%s'\n", old_s);
				}
				/* Since op is '=', clear all bits corresponding to the
				 * wholist, or all file bits if wholist is empty. */
				permlist = (mode_t) ~FILEMODEBITS;
				if (wholist) {
					permlist = ~wholist;
				}
				new_mode &= permlist;
			}
			op = *s++;

			/* Check for permcopy. */
			p = who_chars + 1;  /* Skip 'a' entry. */
			do {
				if (*p == *s) {
					int i = 0;
					permlist = who_mask[(int)(p-who_chars)]
							 & (S_IRWXU | S_IRWXG | S_IRWXO)
							 & new_mode;
					do {
						if (permlist & perm_mask[i]) {
							permlist |= perm_mask[i];
						}
					} while (++i < 3);
					++s;
					goto GOT_ACTION;
				}
			} while (*++p);

			/* It was not a permcopy, so get a permlist. */
			permlist = 0;
 PERM_LIST:
			p = perm_chars;
			do {
				if (*p == *s) {
					if ((*p != 'X')
					 || (new_mode & (S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH))
					) {
						permlist |= perm_mask[(int)(p-perm_chars)];
					}
					if (!*++s) {
						break;
					}
					goto PERM_LIST;
				}
			} while (*++p);
 GOT_ACTION:
			if (permlist) { /* The permlist was nonempty. */
				mode_t tmp = wholist;
				if (!wholist) {
					mode_t u_mask = umask(0);
					umask(u_mask);
					tmp = ~u_mask;
				}
				permlist &= tmp;
				if (op == '-') {
					new_mode &= ~permlist;
				} else {
					new_mode |= permlist;
				}
			}
		} while (*s && (*s != ','));
	}

	return new_mode;
}
