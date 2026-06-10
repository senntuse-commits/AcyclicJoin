# TPC-DS

Our TPC-DS implementation is based on the tpcds-kit project, available at https://github.com/gregrahn/tpcds-kit. We would like to thank everyone involved in this project.

# How to use?

Firstly, move the `tpcds` folder to a directory where the user has read and write permissions and modify the configuration file `tpcds.json`. 

```json
{
    "test_name": "tpchds",
    "pg_ip": "127.0.0.1",
    "pg_port": "5432",
    "pg_user": "postgres",
    "pg_password": "123456",
    "pg_log_dir": "/tmp",
    "data_size": "1",
    "tpcds_file_dir":"/var/tpcds-kit-master/",
    "HEDB_file_dir":"/root/HEDB/",
    "secure_db":"tpcds_test",
    "insecure_db":"test",
    "output":"/var/tpcds-kit-master/output/",
    "querydir":"/var/tpcds-kit-master/enc_sql/"
}
```

Secondly, Run the script (you may need to install python package `psycopg2`).

```bash
$ python3 run.py -c
$ python3 run.py -l
$ python3 run.py -t
```

Thirdly, if you want to generate TPC-DS data files, you can send `tpcds/enc_sql/enc_q0.sql` to PostgreSQL after modifying `HEDB/src/integrity_zone/udf/enc_int4.cpp`.

# Why is `tools/data/a.py` needed?

Because the default generated dataset contains empty data, which can affect queries, we need to clean the data.
