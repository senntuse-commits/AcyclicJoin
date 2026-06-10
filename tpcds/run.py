import argparse
import os
import psycopg2
import json
import subprocess
import time

DEFAULT_TPCHDS_CONFIG = "tpcds.json"
tables = ['time_dim', 'warehouse', 'call_center', 'web_site', 'web_returns', 'web_page', 'store_sales', 'promotion', 'ship_mode', 'catalog_page', 'web_sales', 'customer', 'income_band', 'customer_address', 'catalog_sales', 'store_returns', 'dbgen_version', 'item', 'inventory', 'store', 'household_demographics', 'customer_demographics', 'reason', 'date_dim', 'catalog_returns']

class tpcds():
    def __init__(self,PropFile = DEFAULT_TPCHDS_CONFIG):
        config = self.loadPropertyFile(PropFile)
        self.pgIp = config['pg_ip']
        self.pgPort = config['pg_port']
        self.pgUser = config['pg_user']
        self.pgDB = config["secure_db"]
        self.pgPW = config['pg_password']
        self.outDir = config['output']
        self.queryDir = config['querydir']
        self.datasize = config['data_size']
        self.fileDir = config['tpcds_file_dir']
        self.schema = self.fileDir+'tools/tpcds.sql'
        # self.schema = self.fileDir+'tools/test.sql'

    #创建表格
    def PrepCreateTable(self):
        cmd = f"PGPASSWORD={self.pgPW} psql -h {self.pgIp} -p {self.pgPort} -U {self.pgUser} -f {self.schema}"
        self.executeCommand(cmd)
    
    #加载数据
    def LoadTableData(self,default = True):
        if default:
            #Clean the data and exclude empty data
            cmd = f"cd tools && rm -f data/*.dat && ./dsdgen -sc {self.datasize} -DIR 'data/'&& chmod 777 data/*.dat"
            self.executeCommand(cmd)
            cmd = 'cd tools/data && python3 a.py'
            self.executeCommand(cmd)
            cmd = '''
            cd tools/data && \
            for i in $(ls *.dat); do \
                name=$i; \
                echo $name; \
                sed -i 's#|$##g' $name; \
            done
            '''
            self.executeCommand(cmd)
        for table in tables:
            datPath = f"{self.fileDir}tools/data/{table}.dat"
            cmd = f"PGPASSWORD={self.pgPW} psql -h {self.pgIp} -p {self.pgPort} -U {self.pgUser} -d {self.pgDB} -c \"SELECT enable_client_mode();COPY {table} FROM '{datPath}' WITH DELIMITER AS '|' NULL '';\""
            # cmd = f"PGPASSWORD={self.pgPW} psql -h {self.pgIp} -p {self.pgPort} -U {self.pgUser} -d {self.pgDB} -c \"COPY {table} FROM '{datPath}' WITH DELIMITER AS '|' NULL '';\""
            self.executeCommand(cmd)

    def RunTest(self):
        totalTime = 0
        cmd = 'cd output && rm *.out -f'
        self.executeCommand(cmd)
        for i in range(1,100):
            conn = psycopg2.connect(database = self.pgDB, user = self.pgUser, password = self.pgPW, host = self.pgIp, port = self.pgPort)
            cur = conn.cursor()
            start = time.time()

            query = self.queryDir+f"enc_q{i}.sql"
            outputFile = open(self.outDir+f"enc_q{i}.out","w+")

            try:
                cur.execute("select enable_client_mode();")
                cur.execute(open(query,"r").read())
                if(i!=100):
                    result = cur.fetchall()
                    outputFile.write(str(cur.description)+'\n')
                    for row in result:
                        outputFile.write(str(row) + '\n')
                    outputFile.close()
                conn.commit()
                end = time.time()
                totalTime = totalTime + int((end - start) * 1000)
                print(f"query enc_q{i}.sql: {int((end - start) * 1000)}ms")

                cur.close()
                conn.close()
            except Exception as e:
                print(f"An error occurred: {e}")
        print(f"Total time:{totalTime}ms")

    # 加载配置文件
    def loadPropertyFile(self,pFileName):
        data = {}
        try:
            json_data = open(pFileName)
        except:
            print("File does not exist " + pFileName)
        try:
            data = json.load(json_data)
        except:
            print("Incorrect JSON Format  " + pFileName)
            raise
        return data

    # 执行cmd指令
    def executeCommand(self,command, printfn=print, printCmd=True, prefix=""):
        if printCmd and 'sed' not in command:
            printfn("Calling %s" % command)
        if prefix:
            prefix = " (%s)" % prefix
        try:
            with subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT) as p:
                for line in iter(p.stdout.readline, b''):
                    printfn('>>>%s %s' % (prefix, str(line, "utf-8").rstrip()))
        except Exception as e:
            printfn("Terminated " +  command + " " + str(e))

    #主函数
    def main(self):
        parser = argparse.ArgumentParser(description='Run tpcds.')
        parser.add_argument('-c', '--CreateTable',action='store_true',help='only crate table by tpcds.sql')
        parser.add_argument('-l', '--LoadData',action='store_true',help='generate data and load data by *.dat')
        parser.add_argument('-t', '--Test',action='store_true',help='only run test by query*.sql')
        parser.add_argument('-sg', '--SkipGenereate',action='store_true',help='only load data by *.dat')
        args = parser.parse_args()
        if args.CreateTable:
            self.PrepCreateTable()
        elif args.LoadData:
            self.LoadTableData()
        elif args.SkipGenereate:
            self.LoadTableData(False)
        elif args.Test:
            self.RunTest()
        else:
            parser.print_help()
            return

if __name__ == "__main__":
    tpcds().main()
