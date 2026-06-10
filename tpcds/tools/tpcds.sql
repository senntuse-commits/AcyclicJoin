-- 
-- Legal Notice 
-- 
-- This document and associated source code (the "Work") is a part of a 
-- benchmark specification maintained by the TPC. 
-- 
-- The TPC reserves all right, title, and interest to the Work as provided 
-- under U.S. and international laws, including without limitation all patent 
-- and trademark rights therein. 
-- 
-- No Warranty 
-- 
-- 1.1 TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, THE INFORMATION 
--     CONTAINED HEREIN IS PROVIDED "AS IS" AND WITH ALL FAULTS, AND THE 
--     AUTHORS AND DEVELOPERS OF THE WORK HEREBY DISCLAIM ALL OTHER 
--     WARRANTIES AND CONDITIONS, EITHER EXPRESS, IMPLIED OR STATUTORY, 
--     INCLUDING, BUT NOT LIMITED TO, ANY (IF ANY) IMPLIED WARRANTIES, 
--     DUTIES OR CONDITIONS OF MERCHANTABILITY, OF FITNESS FOR A PARTICULAR 
--     PURPOSE, OF ACCURACY OR COMPLETENESS OF RESPONSES, OF RESULTS, OF 
--     WORKMANLIKE EFFORT, OF LACK OF VIRUSES, AND OF LACK OF NEGLIGENCE. 
--     ALSO, THERE IS NO WARRANTY OR CONDITION OF TITLE, QUIET ENJOYMENT, 
--     QUIET POSSESSION, CORRESPONDENCE TO DESCRIPTION OR NON-INFRINGEMENT 
--     WITH REGARD TO THE WORK. 
-- 1.2 IN NO EVENT WILL ANY AUTHOR OR DEVELOPER OF THE WORK BE LIABLE TO 
--     ANY OTHER PARTY FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO THE 
--     COST OF PROCURING SUBSTITUTE GOODS OR SERVICES, LOST PROFITS, LOSS 
--     OF USE, LOSS OF DATA, OR ANY INCIDENTAL, CONSEQUENTIAL, DIRECT, 
--     INDIRECT, OR SPECIAL DAMAGES WHETHER UNDER CONTRACT, TORT, WARRANTY,
--     OR OTHERWISE, ARISING IN ANY WAY OUT OF THIS OR ANY OTHER AGREEMENT 
--     RELATING TO THE WORK, WHETHER OR NOT SUCH AUTHOR OR DEVELOPER HAD 
--     ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES. 
-- 
-- Contributors:
-- Gradient Systems
--

DROP DATABASE IF EXISTS tpcds_test;
CREATE DATABASE tpcds_test;
\c tpcds_test

DROP EXTENSION IF EXISTS hedb;
CREATE EXTENSION hedb;
ALTER SYSTEM SET max_parallel_workers_per_gather = 0; -- FIXME: avoid replay bug

create table dbgen_version
(
    dv_version                varchar(16)                   ,
    dv_create_date            date                          ,
    dv_create_time            time                          ,
    dv_cmdline_args           varchar(200)                  
);

create table customer_address
(
    ca_address_sk             enc_int4               not null,
    ca_address_id             enc_text              not null,
    ca_street_number          enc_text                      ,
    ca_street_name            enc_text                      ,
    ca_street_type            enc_text                      ,
    ca_suite_number           enc_text                      ,
    ca_city                   enc_text                      ,
    ca_county                 enc_text                      ,
    ca_state                  enc_text                      ,
    ca_zip                    enc_text                      ,
    ca_country                enc_text                      ,
    ca_gmt_offset             enc_float4                    ,
    ca_location_type          enc_text                      ,
    primary key (ca_address_sk)
);

create table customer_demographics
(
    cd_demo_sk                enc_int4               not null,
    cd_gender                 enc_text                       ,
    cd_marital_status         enc_text                       ,
    cd_education_status       enc_text                       ,
    cd_purchase_estimate      enc_int4                       ,
    cd_credit_rating          enc_text                       ,
    cd_dep_count              enc_int4                       ,
    cd_dep_employed_count     enc_int4                       ,
    cd_dep_college_count      enc_int4                       ,
    primary key (cd_demo_sk)
);

create table date_dim
(
    d_date_sk                 enc_int4               not null,
    d_date_id                 enc_text              not null,
    d_date                    enc_timestamp                 ,
    d_month_seq               enc_int4                      ,
    d_week_seq                enc_int4                      ,
    d_quarter_seq             enc_int4                      ,
    d_year                    enc_int4                      ,
    d_dow                     enc_int4                      ,
    d_moy                     enc_int4                      ,
    d_dom                     enc_int4                      ,
    d_qoy                     enc_int4                      ,
    d_fy_year                 enc_int4                      ,
    d_fy_quarter_seq          enc_int4                      ,
    d_fy_week_seq             enc_int4                      ,
    d_day_name                enc_text                      ,
    d_quarter_name            enc_text                      ,
    d_holiday                 enc_text                      ,
    d_weekend                 enc_text                      ,
    d_following_holiday       enc_text                      ,
    d_first_dom               enc_int4                      ,
    d_last_dom                enc_int4                      ,
    d_same_day_ly             enc_int4                      ,
    d_same_day_lq             enc_int4                      ,
    d_current_day             enc_text                      ,
    d_current_week            enc_text                      ,
    d_current_month           enc_text                      ,
    d_current_quarter         enc_text                      ,
    d_current_year            enc_text                      ,
    primary key (d_date_sk)
);

create table warehouse
(
    w_warehouse_sk            enc_int4               not null,
    w_warehouse_id            enc_text              not null,
    w_warehouse_name          enc_text                      ,
    w_warehouse_sq_ft         enc_int4                      ,
    w_street_number           enc_text                      ,
    w_street_name             enc_text                      ,
    w_street_type             enc_text                      ,
    w_suite_number            enc_text                      ,
    w_city                    enc_text                      ,
    w_county                  enc_text                      ,
    w_state                   enc_text                      ,
    w_zip                     enc_text                      ,
    w_country                 enc_text                      ,
    w_gmt_offset              enc_float4                    ,
    primary key (w_warehouse_sk)
);

create table ship_mode
(
    sm_ship_mode_sk           enc_int4               not null,
    sm_ship_mode_id           enc_text              not null,
    sm_type                   enc_text                      ,
    sm_code                   enc_text                      ,
    sm_carrier                enc_text                      ,
    sm_contract               enc_text                      ,
    primary key (sm_ship_mode_sk)
);

create table time_dim
(
    t_time_sk                 enc_int4               not null,
    t_time_id                 enc_text              not null,
    t_time                    enc_int4                      ,
    t_hour                    enc_int4                      ,
    t_minute                  enc_int4                      ,
    t_second                  enc_int4                      ,
    t_am_pm                   enc_text                      ,
    t_shift                   enc_text                      ,
    t_sub_shift               enc_text                      ,
    t_meal_time               enc_text                      ,
    primary key (t_time_sk)
);

create table reason
(
    r_reason_sk               enc_int4               not null,
    r_reason_id               enc_text              not null,
    r_reason_desc             enc_text                      ,
    primary key (r_reason_sk)
);

create table income_band
(
    ib_income_band_sk         enc_int4               not null,
    ib_lower_bound            enc_int4                      ,
    ib_upper_bound            enc_int4                      ,
    primary key (ib_income_band_sk)
);

create table item
(
    i_item_sk                 enc_int4               not null,
    i_item_id                 enc_text              not null,
    i_rec_start_date          enc_timestamp                          ,
    i_rec_end_date            enc_timestamp                          ,
    i_item_desc               enc_text                  ,
    i_current_price           enc_float4                  ,
    i_wholesale_cost          enc_float4                  ,
    i_brand_id                enc_int4                       ,
    i_brand                   enc_text                      ,
    i_class_id                enc_int4                       ,
    i_class                   enc_text                      ,
    i_category_id             enc_int4                       ,
    i_category                enc_text                      ,
    i_manufact_id             enc_int4                       ,
    i_manufact                enc_text                      ,
    i_size                    enc_text                      ,
    i_formulation             enc_text                      ,
    i_color                   enc_text                      ,
    i_units                   enc_text                      ,
    i_container               enc_text                      ,
    i_manager_id              enc_int4                       ,
    i_product_name            enc_text                      ,
    primary key (i_item_sk)
);

create table store
(
    s_store_sk                enc_int4               not null,
    s_store_id                enc_text              not null,
    s_rec_start_date          enc_timestamp                          ,
    s_rec_end_date            enc_timestamp                          ,
    s_closed_date_sk          enc_int4                       ,
    s_store_name              enc_text                   ,
    s_number_employees        enc_int4                       ,
    s_floor_space             enc_int4                       ,
    s_hours                   enc_text                   ,
    s_manager                 enc_text                   ,
    s_market_id               enc_int4                      ,
    s_geography_class         enc_text                  ,
    s_market_desc             enc_text                  ,
    s_market_manager          enc_text                  ,
    s_division_id             enc_int4                       ,
    s_division_name           enc_text                   ,
    s_company_id              enc_int4                       ,
    s_company_name            enc_text                   ,
    s_street_number           enc_text                   ,
    s_street_name             enc_text                   ,
    s_street_type             enc_text                   ,
    s_suite_number            enc_text                   ,
    s_city                    enc_text                   ,
    s_county                  enc_text                   ,
    s_state                   enc_text                   ,
    s_zip                     enc_text                   ,
    s_country                 enc_text                   ,
    s_gmt_offset              enc_float4                  ,
    s_tax_precentage          enc_float4                  ,
    primary key (s_store_sk)
);

create table call_center
(
    cc_call_center_sk         enc_int4               not null,
    cc_call_center_id         enc_text              not null,
    cc_rec_start_date         enc_timestamp                          ,
    cc_rec_end_date           enc_timestamp                          ,
    cc_closed_date_sk         enc_int4                       ,
    cc_open_date_sk           enc_int4                       ,
    cc_name                   enc_text                   ,
    cc_class                  enc_text                   ,
    cc_employees              enc_int4                       ,
    cc_sq_ft                  enc_int4                       ,
    cc_hours                  enc_text                   ,
    cc_manager                enc_text                   ,
    cc_mkt_id                 enc_int4                       ,
    cc_mkt_class              enc_text                  ,
    cc_mkt_desc               enc_text                  ,
    cc_market_manager         enc_text                  ,
    cc_division               enc_int4                       ,
    cc_division_name          enc_text                   ,
    cc_company                enc_int4                       ,
    cc_company_name           enc_text                   ,
    cc_street_number          enc_text                   ,
    cc_street_name            enc_text                   ,
    cc_street_type            enc_text                   ,
    cc_suite_number           enc_text                   ,
    cc_city                   enc_text                   ,
    cc_county                 enc_text                   ,
    cc_state                  enc_text                   ,
    cc_zip                    enc_text                   ,
    cc_country                enc_text                   ,
    cc_gmt_offset             enc_float4                  ,
    cc_tax_percentage         enc_float4                  ,
    primary key (cc_call_center_sk)
);

create table customer
(
    c_customer_sk             enc_int4               not null,
    c_customer_id             enc_text              not null,
    c_current_cdemo_sk        enc_int4                      ,
    c_current_hdemo_sk        enc_int4                      ,
    c_current_addr_sk         enc_int4                      ,
    c_first_shipto_date_sk    enc_int4                      ,
    c_first_sales_date_sk     enc_int4                      ,
    c_salutation              enc_text                      ,
    c_first_name              enc_text                      ,
    c_last_name               enc_text                      ,
    c_preferred_cust_flag     enc_text                      ,
    c_birth_day               enc_int4                       ,
    c_birth_month             enc_int4                       ,
    c_birth_year              enc_int4                       ,
    c_birth_country           enc_text                   ,
    c_login                   enc_text                   ,
    c_email_address           enc_text                   ,
    c_last_review_date_sk     enc_int4                      ,
    primary key (c_customer_sk)
);

create table web_site
(
    web_site_sk               enc_int4               not null,
    web_site_id               enc_text              not null,
    web_rec_start_date        enc_timestamp                          ,
    web_rec_end_date          enc_timestamp                          ,
    web_name                  enc_text                   ,
    web_open_date_sk          enc_int4                       ,
    web_close_date_sk         enc_int4                       ,
    web_class                 enc_text                   ,
    web_manager               enc_text                   ,
    web_mkt_id                enc_int4                      ,
    web_mkt_class             enc_text                   ,
    web_mkt_desc              enc_text                 ,
    web_market_manager        enc_text                   ,
    web_company_id            enc_int4                      ,
    web_company_name          enc_text                   ,
    web_street_number         enc_text                   ,
    web_street_name           enc_text                   ,
    web_street_type           enc_text                   ,
    web_suite_number          enc_text                   ,
    web_city                  enc_text                   ,
    web_county                enc_text                   ,
    web_state                 enc_text                   ,
    web_zip                   enc_text                   ,
    web_country               enc_text                   ,
    web_gmt_offset            enc_float4                  ,
    web_tax_percentage        enc_float4                  ,
    primary key (web_site_sk)
);

create table store_returns
(
    sr_returned_date_sk       enc_int4                       ,
    sr_return_time_sk         enc_int4                       ,
    sr_item_sk                enc_int4               not null,
    sr_customer_sk            enc_int4                       ,
    sr_cdemo_sk               enc_int4                       ,
    sr_hdemo_sk               enc_int4                       ,
    sr_addr_sk                enc_int4                       ,
    sr_store_sk               enc_int4                       ,
    sr_reason_sk              enc_int4                       ,
    sr_ticket_number          enc_int4               not null,
    sr_return_quantity        enc_int4                       ,
    sr_return_amt             enc_float4                  ,
    sr_return_tax             enc_float4                  ,
    sr_return_amt_inc_tax     enc_float4                  ,
    sr_fee                    enc_float4                  ,
    sr_return_ship_cost       enc_float4                  ,
    sr_refunded_cash          enc_float4                  ,
    sr_reversed_charge        enc_float4                  ,
    sr_store_credit           enc_float4                  ,
    sr_net_loss               enc_float4                  ,
    primary key (sr_item_sk, sr_ticket_number)
);

create table household_demographics
(
    hd_demo_sk                enc_int4               not null,
    hd_income_band_sk         enc_int4                      ,
    hd_buy_potential          enc_text                      ,
    hd_dep_count              enc_int4                       ,
    hd_vehicle_count          enc_int4                       ,
    primary key (hd_demo_sk)
);

create table web_page
(
    wp_web_page_sk            enc_int4               not null,
    wp_web_page_id            enc_text              not null,
    wp_rec_start_date         enc_timestamp                          ,
    wp_rec_end_date           enc_timestamp                          ,
    wp_creation_date_sk       enc_int4                       ,
    wp_access_date_sk         enc_int4                       ,
    wp_autogen_flag           enc_text                       ,
    wp_customer_sk            enc_int4                       ,
    wp_url                    enc_text                  ,
    wp_type                   enc_text                      ,
    wp_char_count             enc_int4                       ,
    wp_link_count             enc_int4                      ,
    wp_image_count            enc_int4                       ,
    wp_max_ad_count           enc_int4                      ,
    primary key (wp_web_page_sk)
);

create table promotion
(
    p_promo_sk                enc_int4               not null,
    p_promo_id                enc_text              not null,
    p_start_date_sk           enc_int4                       ,
    p_end_date_sk             enc_int4                       ,
    p_item_sk                 enc_int4                       ,
    p_cost                    enc_float4                 ,
    p_response_target         enc_int4                       ,
    p_promo_name              enc_text                  ,
    p_channel_dmail           enc_text                  ,
    p_channel_email           enc_text                  ,
    p_channel_catalog         enc_text                  ,
    p_channel_tv              enc_text                  ,
    p_channel_radio           enc_text                  ,
    p_channel_press           enc_text                  ,
    p_channel_event           enc_text                  ,
    p_channel_demo            enc_text                  ,
    p_channel_details         enc_text                  ,
    p_purpose                 enc_text                  ,
    p_discount_active         enc_text                  ,
    primary key (p_promo_sk)
);

create table catalog_page
(
    cp_catalog_page_sk        enc_int4               not null,
    cp_catalog_page_id        enc_text              not null,
    cp_start_date_sk          enc_int4                       ,
    cp_end_date_sk            enc_int4                       ,
    cp_department             enc_text                   ,
    cp_catalog_number         enc_int4                       ,
    cp_catalog_page_number    enc_int4                       ,
    cp_description            enc_text                  ,
    cp_type                   enc_text                  ,
    primary key (cp_catalog_page_sk)
);

create table inventory
(
    inv_date_sk               enc_int4               not null,
    inv_item_sk               enc_int4               not null,
    inv_warehouse_sk          enc_int4               not null,
    inv_quantity_on_hand      enc_int4                       ,
    primary key (inv_date_sk, inv_item_sk, inv_warehouse_sk)
);

create table catalog_returns
(
    cr_returned_date_sk       enc_int4                       ,
    cr_returned_time_sk       enc_int4                       ,
    cr_item_sk                enc_int4               not null,
    cr_refunded_customer_sk   enc_int4                       ,
    cr_refunded_cdemo_sk      enc_int4                       ,
    cr_refunded_hdemo_sk      enc_int4                       ,
    cr_refunded_addr_sk       enc_int4                       ,
    cr_returning_customer_sk  enc_int4                       ,
    cr_returning_cdemo_sk     enc_int4                       ,
    cr_returning_hdemo_sk     enc_int4                       ,
    cr_returning_addr_sk      enc_int4                       ,
    cr_call_center_sk         enc_int4                       ,
    cr_catalog_page_sk        enc_int4                       ,
    cr_ship_mode_sk           enc_int4                       ,
    cr_warehouse_sk           enc_int4                       ,
    cr_reason_sk              enc_int4                       ,
    cr_order_number           enc_int4               not null,
    cr_return_quantity        enc_int4                       ,
    cr_return_amount          enc_float4                  ,
    cr_return_tax             enc_float4                  ,
    cr_return_amt_inc_tax     enc_float4                  ,
    cr_fee                    enc_float4                  ,
    cr_return_ship_cost       enc_float4                  ,
    cr_refunded_cash          enc_float4                  ,
    cr_reversed_charge        enc_float4                  ,
    cr_store_credit           enc_float4                  ,
    cr_net_loss               enc_float4                  ,
    primary key (cr_item_sk, cr_order_number)
);

create table web_returns
(
    wr_returned_date_sk       enc_int4                       ,
    wr_returned_time_sk       enc_int4                       ,
    wr_item_sk                enc_int4               not null,
    wr_refunded_customer_sk   enc_int4                       ,
    wr_refunded_cdemo_sk      enc_int4                       ,
    wr_refunded_hdemo_sk      enc_int4                       ,
    wr_refunded_addr_sk       enc_int4                       ,
    wr_returning_customer_sk  enc_int4                       ,
    wr_returning_cdemo_sk     enc_int4                       ,
    wr_returning_hdemo_sk     enc_int4                       ,
    wr_returning_addr_sk      enc_int4                       ,
    wr_web_page_sk            enc_int4                       ,
    wr_reason_sk              enc_int4                       ,
    wr_order_number           enc_int4               not null,
    wr_return_quantity        enc_int4                       ,
    wr_return_amt             enc_float4                  ,
    wr_return_tax             enc_float4                  ,
    wr_return_amt_inc_tax     enc_float4                  ,
    wr_fee                    enc_float4                  ,
    wr_return_ship_cost       enc_float4                  ,
    wr_refunded_cash          enc_float4                  ,
    wr_reversed_charge        enc_float4                  ,
    wr_account_credit         enc_float4                  ,
    wr_net_loss               enc_float4                  ,
    primary key (wr_item_sk, wr_order_number)
);

create table web_sales
(
    ws_sold_date_sk           enc_int4                       ,
    ws_sold_time_sk           enc_int4                       ,
    ws_ship_date_sk           enc_int4                       ,
    ws_item_sk                enc_int4               not null,
    ws_bill_customer_sk       enc_int4                       ,
    ws_bill_cdemo_sk          enc_int4                       ,
    ws_bill_hdemo_sk          enc_int4                       ,
    ws_bill_addr_sk           enc_int4                       ,
    ws_ship_customer_sk       enc_int4                       ,
    ws_ship_cdemo_sk          enc_int4                       ,
    ws_ship_hdemo_sk          enc_int4                       ,
    ws_ship_addr_sk           enc_int4                       ,
    ws_web_page_sk            enc_int4                       ,
    ws_web_site_sk            enc_int4                       ,
    ws_ship_mode_sk           enc_int4                       ,
    ws_warehouse_sk           enc_int4                       ,
    ws_promo_sk               enc_int4                       ,
    ws_order_number           enc_int4               not null,
    ws_quantity               enc_int4                       ,
    ws_wholesale_cost         enc_float4                  ,
    ws_list_price             enc_float4                  ,
    ws_sales_price            enc_float4                  ,
    ws_ext_discount_amt       enc_float4                  ,
    ws_ext_sales_price        enc_float4                  ,
    ws_ext_wholesale_cost     enc_float4                  ,
    ws_ext_list_price         enc_float4                  ,
    ws_ext_tax                enc_float4                  ,
    ws_coupon_amt             enc_float4                  ,
    ws_ext_ship_cost          enc_float4                  ,
    ws_net_paid               enc_float4                  ,
    ws_net_paid_inc_tax       enc_float4                  ,
    ws_net_paid_inc_ship      enc_float4                  ,
    ws_net_paid_inc_ship_tax  enc_float4                  ,
    ws_net_profit             enc_float4                  ,
    primary key (ws_item_sk, ws_order_number)
);

create table catalog_sales
(
    cs_sold_date_sk           enc_int4                       ,
    cs_sold_time_sk           enc_int4                       ,
    cs_ship_date_sk           enc_int4                       ,
    cs_bill_customer_sk       enc_int4                       ,
    cs_bill_cdemo_sk          enc_int4                       ,
    cs_bill_hdemo_sk          enc_int4                       ,
    cs_bill_addr_sk           enc_int4                       ,
    cs_ship_customer_sk       enc_int4                       ,
    cs_ship_cdemo_sk          enc_int4                       ,
    cs_ship_hdemo_sk          enc_int4                       ,
    cs_ship_addr_sk           enc_int4                       ,
    cs_call_center_sk         enc_int4                       ,
    cs_catalog_page_sk        enc_int4                       ,
    cs_ship_mode_sk           enc_int4                       ,
    cs_warehouse_sk           enc_int4                       ,
    cs_item_sk                enc_int4               not null,
    cs_promo_sk               enc_int4                       ,
    cs_order_number           enc_int4               not null,
    cs_quantity               enc_int4                       ,
    cs_wholesale_cost         enc_float4                  ,
    cs_list_price             enc_float4                  ,
    cs_sales_price            enc_float4                  ,
    cs_ext_discount_amt       enc_float4                  ,
    cs_ext_sales_price        enc_float4                  ,
    cs_ext_wholesale_cost     enc_float4                  ,
    cs_ext_list_price         enc_float4                  ,
    cs_ext_tax                enc_float4                  ,
    cs_coupon_amt             enc_float4                  ,
    cs_ext_ship_cost          enc_float4                  ,
    cs_net_paid               enc_float4                  ,
    cs_net_paid_inc_tax       enc_float4                  ,
    cs_net_paid_inc_ship      enc_float4                  ,
    cs_net_paid_inc_ship_tax  enc_float4                  ,
    cs_net_profit             enc_float4                  ,
    primary key (cs_item_sk, cs_order_number)
);

create table store_sales
(
    ss_sold_date_sk           enc_int4                       ,
    ss_sold_time_sk           enc_int4                       ,
    ss_item_sk                enc_int4               not null,
    ss_customer_sk            enc_int4                       ,
    ss_cdemo_sk               enc_int4                       ,
    ss_hdemo_sk               enc_int4                       ,
    ss_addr_sk                enc_int4                       ,
    ss_store_sk               enc_int4                       ,
    ss_promo_sk               enc_int4                       ,
    ss_ticket_number          enc_int4               not null,
    ss_quantity               enc_int4                       ,
    ss_wholesale_cost         enc_float4                  ,
    ss_list_price             enc_float4                  ,
    ss_sales_price            enc_float4                  ,
    ss_ext_discount_amt       enc_float4                  ,
    ss_ext_sales_price        enc_float4                 ,
    ss_ext_wholesale_cost     enc_float4                  ,
    ss_ext_list_price         enc_float4                  ,
    ss_ext_tax                enc_float4                  ,
    ss_coupon_amt             enc_float4                  ,
    ss_net_paid               enc_float4                  ,
    ss_net_paid_inc_tax       enc_float4                  ,
    ss_net_profit             enc_float4                  ,
    primary key (ss_item_sk, ss_ticket_number)
);

