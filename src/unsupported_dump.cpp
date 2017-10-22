bool GetAllFandoms(QHash<QString, int>& fandoms)
{
    QSqlDatabase db = QSqlDatabase::database();
    QString qs = QString("select id, fandom from fandoms");
    QSqlQuery q(db);
    q.prepare(qs);
    if(!ExecAndCheck(q))
        return false;

    while(q.next())
        fandoms[q.value("fandom").toString()] = q.value("id").toInt();
    return true;
}



bool EnsureFandomsNormalized()
{
    QHash<QString, int> fandoms;
    auto result = GetAllFandoms(fandoms);
    if(!result)
        return false;
    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();

    QSqlQuery innerQ(db);
    QString qs = QString(" delete from ficfandoms ");
    innerQ.prepare(qs);
    if(!ExecAndCheck(innerQ))
    {
        db.rollback();
        return false;
    }


    qs = QString("select id, fandom1, fandom2 from fanfics");
    QSqlQuery q(db);
    q.prepare(qs);
    if(!ExecAndCheck(q))
        return false;


    int i = 0;
    while(q.next())
    {
        QSqlQuery innerQ(db);
        QString qs = QString(" insert into ficfandoms(fic_id, fandom_id) values(:fic_id, :fandom_id)  ");
        innerQ.prepare(qs);

        QString firstFandom = q.value("fandom1").toString().trimmed();
        QString secondFandom = q.value("fandom2").toString().trimmed();

        bool failureAtFirst = !firstFandom.isEmpty() && !EnsureFandomExists({firstFandom}, fandoms);
        bool failureAtSecond= !secondFandom.isEmpty() && !EnsureFandomExists({secondFandom}, fandoms);
        if(failureAtFirst || failureAtSecond)
            return false;

        innerQ.bindValue(":fandom_id", fandoms[firstFandom]);
        innerQ.bindValue(":fic_id", q.value("id").toInt());
        if(!ExecAndCheck(innerQ))
        {
            db.rollback();
            return false;
        }
        if(!secondFandom.isEmpty() && firstFandom!=secondFandom)
        {
            innerQ.bindValue(":fandom_id", fandoms[secondFandom]);
            innerQ.bindValue(":fic_id", q.value("id").toInt());
            if(!ExecAndCheck(innerQ))
            {
                db.rollback();
                return false;
            }
        }
        i++;
        if(i%1000 == 0)
            qDebug() << "tick: " << i/1000;
    }
    db.commit();
    return true;
}



//bool EnsureTagForRecommendations()
//{
//    QSqlDatabase db = QSqlDatabase::database();
//    QString qs = QString("PRAGMA table_info(recommendations)");
//    QSqlQuery q(db);
//    q.prepare(qs);
//    q.exec();
//    while(q.next())
//    {
//        if(q.value("name").toString() == "tag")
//            return true;
//    }
//    if(q.lastError().isValid())
//    {
//        qDebug() << q.lastError();
//        qDebug() << q.lastQuery();
//        return false;
//    }

//    bool result =  ExecuteQueryChain(q,
//                     {"CREATE TABLE if not exists Recommendations2(recommender_id INTEGER NOT NULL , fic_id INTEGER NOT NULL , tag varchar default 'core', PRIMARY KEY (recommender_id, fic_id, tag) );",
//                      "insert into Recommendations2 (recommender_id,fic_id, tag) SELECT recommender_id, fic_id, 'core' from Recommendations;",
//                      "DROP TABLE Recommendations;",
//                      "ALTER TABLE Recommendations2 RENAME TO Recommendations;",
//                      "drop index i_recommendations;",
//                      "CREATE INDEX if not exists  I_RECOMMENDATIONS ON Recommendations (recommender_id ASC);"
//                      "CREATE INDEX if not exists  I_RECOMMENDATIONS_TAG ON Recommendations (tag ASC);"
//                      "CREATE INDEX if not exists I_FIC_ID ON Recommendations (fic_id ASC);",
//                      });
//    return result;
//}

bool EnsureFandomExists(core::Fandom fandom, QHash<QString, int> & fandoms)
{
    if(!fandoms.contains(fandom.name))
    {
        auto f1ID= CreateIDForFandom(fandom);
        if(f1ID < 0)
        {
            qDebug() << fandom.name << " doesnt exist and ouldnt be created";
            return false;
        }

        fandoms[fandom.name] = f1ID;
    }
    return true;
}

bool WriteFandomsForStory(core::Fic &section, QHash<QString, int> & fandoms)
{
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery innerQ(db);
    QString qs = QString(" delete from ficfandoms where fic_id = (select id from fanfics where %1_id = :fic_id)");
    qs=qs.arg(section.webSite);
    innerQ.prepare(qs);
    innerQ.bindValue(":fic_id", section.webId);
    if(!ExecAndCheck(innerQ))
    {
        db.rollback();
        return false;
    }

    qs = QString(" insert into ficfandoms(fic_id, fandom_id) "
                 " values((select id from fanfics where %1_id = :fic_id), :fandom_id)  ");
    qs=qs.arg(section.webSite);
    innerQ.prepare(qs);

    QString firstFandom = section.fandoms.size() > 0 ? section.fandoms.at(0).trimmed() : "";
    QString secondFandom = section.fandoms.size() > 1 ? section.fandoms.at(1).trimmed() : "";

    bool failureAtFirst = !firstFandom.isEmpty() && !EnsureFandomExists({firstFandom}, fandoms);
    bool failureAtSecond= !secondFandom.isEmpty() && !EnsureFandomExists({secondFandom}, fandoms);
    if(failureAtFirst || failureAtSecond)
        return false;


    innerQ.bindValue(":fandom_id", fandoms[firstFandom]);
    innerQ.bindValue(":fic_id", section.webId);
    if(!ExecAndCheck(innerQ))
    {
        db.rollback();
        return false;
    }
    if(!secondFandom.isEmpty() && firstFandom!=secondFandom)
    {
        innerQ.bindValue(":fandom_id", fandoms[secondFandom]);
        innerQ.bindValue(":fic_id", section.webId);
        if(!ExecAndCheck(innerQ))
        {
            db.rollback();
            return false;
        }
    }

    qs = QString(" update fanfics set fandom1 = :fandom1, fandom2 = :fandom2 "
                 " where %1_id = :fic_id");
    qs=qs.arg(section.webSite);
    innerQ.prepare(qs);
    innerQ.bindValue(":fandom1", firstFandom);
    innerQ.bindValue(":fandom2", secondFandom);
    innerQ.bindValue(":fic_id", section.webId);
    if(!ExecAndCheck(innerQ))
    {
        db.rollback();
        return false;
    }
    return true;
}

void FicParser::WriteJustAuthorName(core::Fic& fic)
{

    fic.author.Log();

    if(fic.author.GetIdStatus() == core::AuthorIdStatus::unassigned)
        fic.author.AssignId(database::GetAuthorIdFromUrl(fic.author.url("ffn")));

    if(fic.author.GetIdStatus() == core::AuthorIdStatus::not_found)
    {
        database::WriteRecommender(fic.author);
        fic.author.AssignId(database::GetAuthorIdFromUrl(fic.author.url("ffn")));
    }
    if(rewriteAuthorName)
        database::AssignNewNameForRecommenderId(fic.author);
}

QList<int> GetFulLRecommenderList()
{
    QList<int> result;

    QSqlDatabase db = QSqlDatabase::database();
    QString qs = QString("select id from recommenders");
    QSqlQuery q(db);
    q.prepare(qs);
    if(!ExecAndCheck(q))
        return result;

    while(q.next())
        result.push_back(q.value(0).toInt());

    return result;
}
