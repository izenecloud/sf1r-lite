#include <search-manager/CustomRanker.h>
#include <search-manager/Sorter.h>

using namespace sf1r;

bool CustomRanker::parse(izenelib::util::UString& ustrExp)
{
    string strExp;
    ustrExp.convertString(strExp, izenelib::util::UString::UTF_8);

    return parse(strExp);
}

bool CustomRanker::parse(std::string& strExp)
{
    trim(strExp); // important

    if (strExp.empty()) {
        errorInfo_ = "Fialed to parse custom_rank[expression], it's empty.";
        return false;
    }

    if (strExp_ != strExp)
        strExp_ = strExp;

    tree_parse_info<> info = ast_parse(strExp.c_str(), customRankTreeParser_, space_p);
    //showBoostAST(info.trees, 0); // test

    if (!info.full) {
        errorInfo_ = "Failed to parse custom_rank[expression], error at \"" + std::string(strExp.c_str(), info.stop) +"^\"";
        return false;
    }

    ESTree_->children_.clear();
    return buildExpSyntaxTree(info.trees, ESTree_);
}

/// private ////////////////////////////////////////////////////////////////////

bool CustomRanker::buildExpSyntaxTree(const ast_info_trees& astrees, ExpSyntaxTreePtr& estree)
{
    for (ast_info_trees::const_iterator iter = astrees.begin(); iter != astrees.end(); ++iter)
    {
        // value in AST
        ExpSyntaxTree::NodeType type = static_cast<ExpSyntaxTree::NodeType>(iter->value.id().to_long());
        ExpSyntaxTreePtr child(new ExpSyntaxTree(type));
        switch (type)
        {
            case ExpSyntaxTree::CONSTANT:
            {
                string v(iter->value.begin(), iter->value.end());
                trim(v);
                //child->value_ = atof(v.c_str());
                if(!str2real(v, child->value_)) {
                    errorInfo_ = "Failed to parse \"" + v + "\" in custom_rank[expression], as a real number. ";
                    return false;
                }
                break;
            }
            case ExpSyntaxTree::PARAMETER:
            {
                // parameter name
                string param(iter->value.begin(), iter->value.end());
                trim(param);

                // parameter value is constant value set by user, or is proptery name
                std::map<std::string, double>::iterator dbIter;
                std::map<std::string, std::string>::iterator proIter;
                if ((dbIter = paramConstValueMap_.find(param)) != paramConstValueMap_.end())
                {
                    // param value is constant
                    child->value_ = dbIter->second;
                    child->type_ = ExpSyntaxTree::CONSTANT;
                }
                else if((proIter = paramPropertyValueMap_.find(param)) != paramPropertyValueMap_.end())
                {
                    // param value is property name
                    child->name_ = proIter->second;
                    propertyDataTypeMap_.insert(std::make_pair(child->name_, UNKNOWN_DATA_PROPERTY_TYPE));
                }
                else
                {
                    // value of param did not found in custom_rank[params],
                    // assume parameter it self is a property name,
                    // all property names will be checked later.
                    child->name_ = param;
                    propertyDataTypeMap_.insert(std::make_pair(child->name_, UNKNOWN_DATA_PROPERTY_TYPE));
                }
                break;
            }
            case ExpSyntaxTree::SUM:
            case ExpSyntaxTree::SUB:
            case ExpSyntaxTree::PRODUCT:
            case ExpSyntaxTree::DIV:
            case ExpSyntaxTree::LOG:
            case ExpSyntaxTree::POW:
            case ExpSyntaxTree::SQRT:
                break;
            default:
                break;
        }

        // children in AST
        if (!buildExpSyntaxTree(iter->children, child)) {
            return false;
        }

        // add to children of EST
        estree->children_.push_back(child);
    }

    return true;
}

bool CustomRanker::setInnerPropertyData(ExpSyntaxTreePtr& estree, SortPropertyCache* pPropertyData)
{
    if (estree->type_ == ExpSyntaxTree::PARAMETER) {
        PropertyDataType dataType = propertyDataTypeMap_[estree->name_];
        estree->propertyData_ = pPropertyData->getSortPropertyData(estree->name_, dataType);
        if (!! estree->propertyData_) {
            stringstream ss;
            ss << "Failed to get sort data for property: " << estree->name_
               << " with data type: " << dataType << endl;
            errorInfo_ = ss.str();
            return false;
        }
    }

    for (est_iterator iter = estree->children_.begin(); iter != estree->children_.end(); iter++)
    {
        if (!setInnerPropertyData(*iter, pPropertyData))
            return false;
    }

    return true;
}

bool CustomRanker::sub_evaluate(ExpSyntaxTreePtr& estree, docid_t& docid)
{
    ExpSyntaxTree::NodeType type = estree->type_;

    switch (type)
    {
        case ExpSyntaxTree::ROOT:
        {
            sub_evaluate(estree->children_[0], docid);
            estree->value_ = estree->children_[0]->value_;
            break;
        }
        case ExpSyntaxTree::CONSTANT:
        {
            // constant value has been set already
            return true;
        }
        case ExpSyntaxTree::PARAMETER:
        {
            if (! estree->propertyData_)
                break;

            void* data = estree->propertyData_->data_;
            switch(estree->propertyData_->type_)
            {
                case INT_PROPERTY_TYPE:
                    estree->value_ = ((int64_t*)data)[docid];
                    break;
                case UNSIGNED_INT_PROPERTY_TYPE:
                    estree->value_ = ((uint64_t*)data)[docid];
                    break;
                case FLOAT_PROPERTY_TYPE:
                    estree->value_ = ((float*)data)[docid];
                    break;
                case DOUBLE_PROPERTY_TYPE:
                    estree->value_ = ((double*)data)[docid];
                    break;
                default:
                    break;
            }
            //cout << "CustomRanker::sub_evaluate docid(" << docid << ") get property \"" << estree->name_ << "\" data value: " << estree->value_ <<endl;
            break;
        }
        case ExpSyntaxTree::SUM:
        {
            sub_evaluate(estree->children_[0], docid);
            estree->value_ = estree->children_[0]->value_;
            sub_evaluate(estree->children_[1], docid);
            estree->value_ += estree->children_[1]->value_;
            break;
        }
        case ExpSyntaxTree::SUB:
        {
            sub_evaluate(estree->children_[0], docid);
            estree->value_ = estree->children_[0]->value_;
            sub_evaluate(estree->children_[1], docid);
            estree->value_ -= estree->children_[1]->value_;
            break;
        }
        case ExpSyntaxTree::PRODUCT:
        {
            sub_evaluate(estree->children_[0], docid);
            estree->value_ = estree->children_[0]->value_;
            sub_evaluate(estree->children_[1], docid);
            estree->value_ *= estree->children_[1]->value_;
            break;
        }
        case ExpSyntaxTree::DIV:
        {
            sub_evaluate(estree->children_[0], docid);
            estree->value_ = estree->children_[0]->value_;
            sub_evaluate(estree->children_[1], docid);
            if (estree->children_[1]->value_ < 1e-8 && estree->children_[1]->value_ > -1e-8)
            {
                errorInfo_ = "*Waring: division by zero, got zero from \"" + estree->children_[1]->name_ + "\"";
                cout << errorInfo_ << endl;
            }
            estree->value_ /= estree->children_[1]->value_;
            break;
        }
        case ExpSyntaxTree::LOG:
        {
            sub_evaluate(estree->children_[0], docid);
            estree->value_ = std::log(estree->children_[0]->value_);
            break;
        }
        case ExpSyntaxTree::POW:
        {
            sub_evaluate(estree->children_[0], docid);
            sub_evaluate(estree->children_[1], docid);
            estree->value_ = std::pow(estree->children_[0]->value_, estree->children_[1]->value_);
            break;
        }
        case ExpSyntaxTree::SQRT:
        {
            sub_evaluate(estree->children_[0], docid);
            estree->value_ = std::sqrt(estree->children_[0]->value_);
            break;
        }
        default:
            break;
    }

    return true;
}


void CustomRanker::showBoostAST(const ast_info_trees& trees, int level)
{
    if (level == 0) {
        cout << "[ AST (Syntax Tree): ]" << endl;
    }

    for (ast_info_trees::const_iterator iter = trees.begin(); iter != trees.end(); ++iter)
    {
        //tree_node<T>: value, children
        cout << string(level*4, ' ') << "|--["
             << string(iter->value.begin(), iter->value.end()) << " : "
             << ExpSyntaxTree::getTypeString(iter->value.id().to_long()) << "]" << endl;
        showBoostAST(iter->children, level+1);
    }
}

void CustomRanker::showEST(est_trees& trees, int level)
{
    for (est_iterator iter = trees.begin(); iter != trees.end(); iter++)
    {
        stringstream ss;
        ss << string(" : ") << iter->get()->name_ << " : &data = " << iter->get()->propertyData_.get();

        cout << string(level*4, ' ') << "|--["
             << ExpSyntaxTree::getTypeString(iter->get()->type_) << " : " << iter->get()->value_
             << ((iter->get()->type_ == ExpSyntaxTree::PARAMETER) ? ss.str() : "")
             << "]" << endl;

        showEST(iter->get()->children_, level+1);
    }
}

void CustomRanker::printESTree()
{
    cout << "[ EST (Expression Syntax Tree): ]" << endl;
    showEST(ESTree_->children_);
}


