# Panduan Debugging Backend Aester

Environment Variables:
- AESTER_LOG=debug|info|error (default: info)
- AESTER_DRYRUN=1 (mode simulasi: penulisan dicatat ke log level debug, verifikasi dilewati, exit 0, tanpa mutasi sysfs/state)
- AESTER_SYSFS_ROOT=/path (mock sysfs direktori pengujian)
- AESTER_STATE_DIR=/path (mock state direktori pengujian)

Mekanisme Verifikasi & Rollback:
- Konstanta retry verifikasi: VERIFY_MAX_RETRIES = 20, VERIFY_RETRY_DELAY_US = 25000 (maksimal tunggu ~500 ms per target).
- Siklus: Baca nilai awal -> Tulis ke semua target -> Verifikasi semua target -> Jika gagal: Rollback ke nilai awal, hapus cache orig terkait, return exit 4 dengan daftar target gagal pada stderr.
