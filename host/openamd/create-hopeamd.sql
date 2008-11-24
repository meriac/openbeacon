drop user hopeamd cascade;

create user hopeamd 
    identified by HopeAMD 
    default tablespace AMDDATA
    quota unlimited on AMDDATA
    quota unlimited on AMDINDX;

GRANT "CONNECT" TO hopeamd;
GRANT "RESOURCE" TO hopeamd;
grant create view to hopeamd;
grand create materialized view to hopeamd;

/

create sequence hopeamd.amdseq;

/

create table hopeamd.providers (
    id                          number,
    provider                    varchar2(100) not null,
    address                     varchar2(100) not null
);

create unique index hopeamd.idx_providers_pk on hopeamd.providers(id) tablespace amdindx;
alter table hopeamd.providers add(constraint providers_pk primary key(id));

/

create table hopeamd.person (
    id                          number not null,
    location                    varchar2(100),
    email                       varchar2(100),
    handle                      varchar2(100) not null,
    password                    varchar2(100) not null,
    room_number                 number(11),
    phone                       varchar2(50),
    age                         number,
    sex                         number,
    country                     varchar2(50),
    edited                      number,
    provider                    number,
    speaker                     number,
    admin                       number,
    reminder                    number,
    validation                  varchar2(40),
    loggedin                    number
);

create unique index hopeamd.idx_person_pk on hopeamd.person(id) tablespace amdindx;
alter table hopeamd.person add (constraint person_pk primary key (id));
create index hopeamd.idx_person_country on hopeamd.person(country) tablespace amdindx;
alter table hopeamd.person add (constraint person_provider_fk foreign key (provider) references hopeamd.providers(id));

/

create table hopeamd.interests_list (
    interest_id                 number not null,
    interest_name               varchar2(100)
);

create unique index hopeamd.idx_interests_list_pk on hopeamd.interests_list(interest_id) tablespace amdindx;
alter table hopeamd.interests_list add (constraint interests_list_pk primary key (interest_id));

/

create table hopeamd.interests (
    id                          number not null,
    interest                    number not null
);

create unique index hopeamd.idx_interests_pk on hopeamd.interests(id, interest) tablespace amdindx;
alter table hopeamd.interests add (constraint interests_pk primary key (id, interest));
alter table hopeamd.interests add (constraint interests_person_fk foreign key (id) references hopeamd.person(id) on delete cascade);
alter table hopeamd.interests add (constraint interests_interest_fk foreign key (interest) references hopeamd.interests_list(interest_id) on delete cascade);

/

create table hopeamd.rooms (
    id                          number not null,
    room                        varchar2(100) not null,
    time                        date default sysdate not null 
);

create unique index hopeamd.idx_rooms_pk on hopeamd.rooms(id) tablespace amdindx;
alter table hopeamd.rooms add (constraint rooms_pk primary key (id));
create unique index hopeamd.idx_rooms_unique on hopeamd.rooms(room) tablespace amdindx;

/

create table hopeamd.talks (
    id                          number not null,
    speaker_name                varchar2(400),
    talk_title                  varchar2(400) not null,
    abstract                    clob,
    talk_time                   date not null,
    track                       varchar2(10) not null
);

create unique index hopeamd.idx_talks_pk on hopeamd.talks(id) tablespace amdindx;
alter table hopeamd.talks add (constraint talks_pk primary key (id));
create index hopeamd.idx_talks_track on hopeamd.talks(track) tablespace amdindx;
create index hopeamd.idx_talks_time on hopeamd.talks(talk_time) tablespace amdindx;

/

create table hopeamd.talks_interests (
    talk_id                     number not null,
    interest                    number not null
);

create unique index hopeamd.idx_talks_interests_pk on hopeamd.talks_interests(talk_id, interest) tablespace amdindx;
alter table hopeamd.talks_interests add (constraint talks_interests_pk primary key (talk_id, interest));
alter table hopeamd.talks_interests add (constraint talks_interests_person_fk foreign key (talk_id) references hopeamd.talks(id) on delete cascade);
alter table hopeamd.talks_interests add (constraint talks_interests_interest_fk foreign key (interest) references hopeamd.interests_list(interest_id));

/

create table hopeamd.browsing (
    id                          number not null,
    url                         varchar2(2000) not null,
    timestamp                   date not null
);

alter table hopeamd.browsing add (constraint browsing_id_fk foreign key (id) references hopeamd.person(id));

/

create table hopeamd.creation (
    id                          number,
    pin                         number,
    registered                  number,
    timestamp                   date default sysdate not null
);

create unique index hopeamd.idx_creation_pk on hopeamd.creation(id) tablespace amdindx;
alter table hopeamd.creation add (constraint creation_pk primary key (id));

/

create table hopeamd.ping (
    id                          number not null,
    to_id                       number not null,
    pingtimestamp               date not null,
    pingtype                    varchar2(20) not null
);    

create index hopeamd.idx_ping_sender on hopeamd.ping(id, pingtimestamp)  tablespace amdindx;
create index hopeamd.idx_ping_receiver on hopeamd.ping(to_id, pingtimestamp) tablespace amdindx;
alter table hopeamd.ping add (constraint ping_sender foreign key (id) references hopeamd.person(id));
alter table hopeamd.ping add (constraint ping_receiver foreign key (to_id) references hopeamd.person(id));

/

create table hopeamd.talks_list (
    hacker_id                   number not null,
    talk_id                     number not null
);

create unique index hopeamd.idx_talks_list_pk on hopeamd.talks_list(hacker_id, talk_id) tablespace amdindx;
alter table hopeamd.talks_list add (constraint talks_list_pk primary key (hacker_id, talk_id));
alter table hopeamd.talks_list add (constraint talks_list_person_fk foreign key (hacker_id) references hopeamd.person(id) on delete cascade);
alter table hopeamd.talks_list add (constraint talks_list_talk_fk foreign key (talk_id) references hopeamd.talks(id) on delete cascade);

/

create table hopeamd.speakers (
    name                        varchar2(100) not null,
    bio                         clob
);

create unique index hopeamd.idx_speakers_pk on hopeamd.speakers(name) tablespace amdindx;
alter table hopeamd.speakers add (constraint speakers_pk primary key(name));

/

create table hopeamd.game_goal (
    goal_id                     number,
    goal_name                   varchar2(200) not null,
    points                      number not null,
    min_regions                 number
);

create unique index hopeamd.idx_game_goal_pk on hopeamd.game_goal(goal_id) tablespace amdindx;
alter table hopeamd.game_goal add (constraint game_goal_pk primary key (goal_id));

/

create table hopeamd.game_goal_region (
    goal_region_id              number not null,
    goal_id                     number not null,
    start_time                  date not null,
    end_time                    date not null,
    area_id                     varchar2(40) not null
);

create unique index hopeamd.idx_game_goal_region_pk on hopeamd.game_goal_region(goal_region_id) tablespace amdindx;
alter table hopeamd.game_goal_region add (constraint game_goal_region_pk primary key(goal_region_id));
create unique index hopeamd.idx_game_goal_region_unique on hopeamd.game_goal_region(goal_id, start_time, end_time, area_id) tablespace amdindx;
alter table hopeamd.game_goal_region add (constraint game_goal_region_unique unique (goal_id, start_time, end_time, area_id));
alter table hopeamd.game_goal_region add (constraint game_goal_region_goal_fk foreign key (goal_id) references hopeamd.game_goal(goal_id));

/

create table hopeamd.region_presence (
    goal_region_id              number not null,
    tag_id                      number not null,
    presence_timestamp          date not null,
    accounted_for               varchar2(1)
);

create unique index hopeamd.idx_region_presence_pk on hopeamd.region_presence(goal_region_id, tag_id) tablespace amdindx;
alter table hopeamd.region_presence add (constraint region_presence_pk primary key (goal_region_id, tag_id));
create index hopeamd.idx_reg_pres_time_accounted on hopeamd.region_presence(presence_timestamp, accounted_for) tablespace amdindx;

/

create table hopeamd.game_score (
    id                          number,
    person_id                   number,
    score_timestamp             date,
    points                      number,
    goal_id                     number,
    creator                     varchar2(200)
);

create unique index hopeamd.idx_game_score_pk on hopeamd.game_score(id) tablespace amdindx;
alter table hopeamd.game_score add (constraint game_score_pk primary key(id));
alter table hopeamd.game_score add (constraint game_score_tag_fk foreign key (person_id) references hopeamd.person(id));

/

create table hopeamd.game_auto_score_time (
    score_timestamp             date,
    points                      number
);

create index hopeamd.idx_game_auto_score on hopeamd.game_auto_score_time(score_timestamp);

/

create table hopeamd.position_snapshot (
    snapshot_timestamp          date not null,
    tag_id                      number not null,
    area_id                     varchar2(40) not null,
    x                           number,
    y                           number,
    z                           number
);

create unique index hopeamd.idx_position_snapshot_pk on hopeamd.position_snapshot(snapshot_timestamp, tag_id) tablespace amdindx;
alter table hopeamd.position_snapshot add (constraint position_snapshot_pk primary key (snapshot_timestamp, tag_id));
create index hopeamd.idx_pos_snap_time_area on hopeamd.position_snapshot(snapshot_timestamp, area_id) tablespace amdindx;

/

create table hopeamd.forgot (
  id				number not null,
  time				date
);

create unique index hopeamd.forgot_pk on hopeamd.forgot(id) tablespace amdindx;
alter table hopeamd.forgot add (constraint forgot_pk primary key (id));

/

drop materialized view hopeamd.talk_presence; 

create materialized view hopeamd.talk_presence as
with talks2 as (
    select id, track, talk_time as talk_time, talk_title  from talks
)
select  p.id person_id, t.id talk_id
       ,handle, talk_title, track, talk_time
from    hopeamd.person p
       ,talks2 t
       ,hopeamd.position_snapshot s
where   s.area_id = t.track
    and s.tag_id = p.id
    and s.snapshot_timestamp > t.talk_time+5/1440
    and s.snapshot_timestamp < t.talk_time+1/24-5/1440
group by p.id, t.id
,handle, talk_title, track, talk_time
having count(*) > 30
order by p.id, t.id;

create index hopeamd.idx_talk_presence_talk on hopeamd.talk_presence(talk_id);
create index hopeamd.idx_talk_presence_person on hopeamd.talk_presence(person_id);

DECLARE
  X NUMBER;
BEGIN
  SYS.DBMS_JOB.SUBMIT
  ( job       => X 
   ,what      => 'dbms_mview.refresh(''hopeamd.talk_presence'', ''C'');'
   ,next_date => trunc(sysdate, 'HH') + floor((trunc(sysdate, 'MI')-trunc(sysdate, 'HH'))/(5/1440))*5/1440+5/1440+60/86400
   ,interval  => 'TRUNC(SYSDATE+(5/1440),''MI'')+60/86400'
   ,no_parse  => FALSE
  );
  SYS.DBMS_OUTPUT.PUT_LINE('Job Number is: ' || to_char(x));
COMMIT;
END;

/

drop materialized view hopeamd.snapshot_summary;

create materialized view hopeamd.snapshot_summary tablespace amddata as
with snapshots as (
    select  trunc(snapshot_timestamp, 'HH') + floor((trunc(snapshot_timestamp, 'MI')-trunc(snapshot_timestamp, 'HH'))/(5/1440))*5/1440 time_period,
            s.*
    from    hopeamd.position_snapshot s
)
select  time_period,
        tag_id, area_id, 
        count(1)/sum(count(1)) over (partition by tag_id, time_period) normalized_time_in_area
from    snapshots
--where   snapshot_timestamp > sysdate
--    and snapshot_timestamp < trunc(sysdate, 'HH') + floor((trunc(sysdate, 'MI')-trunc(sysdate, 'HH'))/(5/1440))*5/1440
group by time_period, tag_id, area_id 
order by time_period, tag_id, area_id; 

create unique index hopeamd.snapshot_summ_pk on hopeamd.snapshot_summary(time_period, tag_id, area_id);
alter materialized view hopeamd.snapshot_summary add (constraint snapshot_summ_pk primary key(time_period, tag_id, area_id));

-- rebuild the view 5 seconds after the next 5 minute period ends, and repeat every 5 minutes
DECLARE
  X NUMBER;
BEGIN
  SYS.DBMS_JOB.SUBMIT
  ( job       => X 
   ,what      => 'dbms_mview.refresh(''hopeamd.snapshot_summary'', ''C'');'
   ,next_date => trunc(sysdate, 'HH') + floor((trunc(sysdate, 'MI')-trunc(sysdate, 'HH'))/(5/1440))*5/1440+5/1440+5/86400
   ,interval  => 'TRUNC(SYSDATE+(5/1440),''MI'')+5/86400'
   ,no_parse  => FALSE
  );
  SYS.DBMS_OUTPUT.PUT_LINE('Job Number is: ' || to_char(x));
COMMIT;
END;

/

create type hopeamd.varchar2_t as table of varchar2(4000);

create or replace function hopeamd.collect_concat (t in varchar2_t) 
  return varchar2
as 
  ret varchar2(2000) := '';
  i   number;
begin
  i := t.first;
  while i is not null loop
    if ret is not null then
      ret := ret || ', ';
    end if;

    ret := ret || t(i);

    i := t.next(i);
  end loop;

  return ret;
end;

/
