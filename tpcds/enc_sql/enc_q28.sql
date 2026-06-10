-- start query 1 in stream 0 using template query28.tpl
select  *
from (select avg(ss_list_price) B1_LP
            ,count(ss_list_price) B1_CNT
            ,count(distinct ss_list_price) B1_CNTD
      from store_sales
      where ss_quantity between '0' and '5'
        and (ss_list_price between '11' and enc_float4_encrypt('11')+ enc_float4_encrypt('10') 
             or ss_coupon_amt between '460' and enc_float4_encrypt('460')+ enc_float4_encrypt('1000')
             or ss_wholesale_cost between '14' and enc_float4_encrypt('14')+ enc_float4_encrypt('20'))) B1,
     (select avg(ss_list_price) B2_LP
            ,count(ss_list_price) B2_CNT
            ,count(distinct ss_list_price) B2_CNTD
      from store_sales
      where ss_quantity between '6' and '10'
        and (ss_list_price between '91' and enc_float4_encrypt('91')+ enc_float4_encrypt('10')
          or ss_coupon_amt between '1430' and enc_float4_encrypt('1430')+ enc_float4_encrypt('1000')
          or ss_wholesale_cost between '32' and enc_float4_encrypt('32')+ enc_float4_encrypt('20'))) B2,
     (select avg(ss_list_price) B3_LP
            ,count(ss_list_price) B3_CNT
            ,count(distinct ss_list_price) B3_CNTD
      from store_sales
      where ss_quantity between '11' and '15'
        and (ss_list_price between '66' and enc_float4_encrypt('66')+ enc_float4_encrypt('10')
          or ss_coupon_amt between '920' and enc_float4_encrypt('920')+ enc_float4_encrypt('1000')
          or ss_wholesale_cost between '4' and enc_float4_encrypt('4')+ enc_float4_encrypt('20'))) B3,
     (select avg(ss_list_price) B4_LP
            ,count(ss_list_price) B4_CNT
            ,count(distinct ss_list_price) B4_CNTD
      from store_sales
      where ss_quantity between '16' and '20'
        and (ss_list_price between '142' and enc_float4_encrypt('142')+ enc_float4_encrypt('10')
          or ss_coupon_amt between '3054' and enc_float4_encrypt('3054')+ enc_float4_encrypt('1000')
          or ss_wholesale_cost between '80' and enc_float4_encrypt('80')+ enc_float4_encrypt('20'))) B4,
     (select avg(ss_list_price) B5_LP
            ,count(ss_list_price) B5_CNT
            ,count(distinct ss_list_price) B5_CNTD
      from store_sales
      where ss_quantity between '21' and '25'
        and (ss_list_price between '135' and enc_float4_encrypt('135')+ enc_float4_encrypt('10')
          or ss_coupon_amt between '14180' and enc_float4_encrypt('14180')+ enc_float4_encrypt('1000')
          or ss_wholesale_cost between '38' and enc_float4_encrypt('38')+ enc_float4_encrypt('20'))) B5,
     (select avg(ss_list_price) B6_LP
            ,count(ss_list_price) B6_CNT
            ,count(distinct ss_list_price) B6_CNTD
      from store_sales
      where ss_quantity between '26' and '30'
        and (ss_list_price between '28' and enc_float4_encrypt('28')+ enc_float4_encrypt('10')
          or ss_coupon_amt between '2513' and enc_float4_encrypt('2513')+ enc_float4_encrypt('1000')
          or ss_wholesale_cost between '42' and enc_float4_encrypt('42')+ enc_float4_encrypt('20'))) B6
limit 100;

-- end query 1 in stream 0 using template query28.tpl
